import collections.abc
import dataclasses
import enum
import json
import logging
import os.path
import pathlib
import subprocess
import re
import io
import sys
import typing
from config import *

logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger()


def inclusive(min_val: int, max_val: int, step=1):
    return range(min_val, max_val + 1, step)


def drop(li: list, keys: typing.List[str]) -> list:
    li = li.copy()
    for key in keys:
        li.remove(key)
    return li


@dataclasses.dataclass(eq=True, frozen=True, order=True)
class Implementation:
    name: str
    allreduce_algorithm: int = None

    def __str__(self):
        return self.name


allgather = Implementation(name='allgather')
allreduce = Implementation(name='allreduce')
allreduce_native_ring = Implementation(name='allreduce-native-ring', allreduce_algorithm=4)
allreduce_native_rabenseifner = Implementation(name='allreduce-native-rabenseifner', allreduce_algorithm=6)
allreduce_native_basic_linear = Implementation(name='allreduce-native-basic_linear', allreduce_algorithm=1)
allreduce_butterfly = Implementation(name='allreduce-butterfly')
allgather_async = Implementation(name='allgather-async')
allreduce_rabenseifner = Implementation(name='allreduce-rabenseifner')
rabenseifner_gather = Implementation(name='rabenseifner-gather')
rabenseifner_scatter = Implementation(name='rabenseifner-scatter')


@dataclasses.dataclass(eq=True, frozen=True, order=True)
class Configuration:
    n: int
    m: int
    nodes: int
    implementation: Implementation
    repetitions: int = 1  # used for repetitions within a job (-t ${repetitions})
    repetition: int = 0  # used for repeated jobs, probably deprecated
    verify: bool = False

    def __str__(self):
        dim = f'{self.n}' if self.n == self.m else f'{self.n}x{self.m}'
        out = f'{dim}, {self.nodes} nodes, {self.implementation}, {self.repetitions}x'
        if self.verify:
            out += ' [verification]'

        return out

    def command(self):
        args = [
            binary_path,
            '-n', str(self.n),
            '-m', str(self.m),
            '-t', str(self.repetitions),
            '-i', self.implementation.name,
        ]
        if self.verify:
            args.append('-c')

        return args

    def memory_usage(self) -> int:
        if self.n < 8192 and self.m < 8192:
            return 1024 * self.nodes

        return int((self.n * self.m / (2 ** 15)) * self.nodes)

    def runnable(self):
        if self.nodes > 48:
            return False, f'euler supports only up to 48 nodes'

        threshold = 128_000
        if self.memory_usage() > threshold:
            return False, f'too much memory requested ({self.memory_usage()} > {threshold})'

        return True, None


class Runner:
    def run(self, config: Configuration):
        pass

    def collect(self, repetition: int):
        pass

    def verify(self, repetition: int) -> bool:
        return True


class Scheduler:
    def __init__(self, runner: Runner):
        self.configs = []
        self.runner = runner

    def register(self, config):
        if isinstance(config, collections.abc.Iterable):
            self.configs.extend(config)
        else:
            self.configs.append(config)

    def run(self):
        for config in self.configurations():
            self.runner.run(config)

    def configurations(self):
        configs = list(set(self.configs))
        configs.sort()
        return configs


class DryRun(Runner):
    def run(self, config: Configuration):
        logger.info(str(config))

        runnable, reason = config.runnable()
        if not runnable:
            logging.warning(reason)


class LocalRunner(Runner):
    def __init__(self, output: io.TextIOWrapper):
        self.output = output

    def run(self, config: Configuration):
        args = config.command()

        proc = subprocess.run(
            args,
            stdout=subprocess.PIPE,
        )

        self.output.write(proc.stdout.decode())


class Status(enum.Enum):
    DONE = 1
    PENDING = 2
    RUNNING = 3
    FAILED = 4


class EulerRunner(Runner):
    def __init__(self, results_dir):
        self.raw_dir = f"{results_dir}/raw"
        self.parsed_dir = f"{results_dir}/parsed"

        for path in [self.raw_dir, self.parsed_dir]:
            pathlib.Path(path).mkdir(parents=True, exist_ok=True)

    def run(self, config: Configuration):
        runnable, reason = config.runnable()
        if not runnable:
            logger.warning(f'skipping configuration: {reason}')
            return

        mpi_args = []
        if config.implementation.allreduce_algorithm is not None:
            mpi_args.extend([
                '--mca', 'coll_tuned_use_dynamic_rules', '1',
                '--mca', 'coll_tuned_allreduce_algorithm', f'{config.implementation.allreduce_algorithm}'
            ])

        args = [
            'bsub',
            # '-J', f'"{config.__str__()}"',
            '-o', f'{self.raw_dir}/%J',
            '-e', f'{self.raw_dir}/%J.err',
            '-n', str(config.nodes),
            '-R', 'span[ptile=1]',  # use 1 core per node
            '-R', 'select[model==XeonE3_1585Lv5]',  # use Euler III nodes (4 cores)
            '-R', 'select[!ib]',  # disable infiniband (not available on Euler III)
            '-r',  # make jobs retryable
            'mpirun',
            *mpi_args,
            '-np', str(config.nodes),
            *config.command(),
        ]

        logger.debug("executing the following command:")
        logger.debug(" ".join(args))

        proc = subprocess.run(args, stdout=subprocess.PIPE)
        process_output = proc.stdout.decode()
        logger.debug(process_output)

        job_id = re \
            .compile("Job <(.*)> is submitted to queue <normal.4h>.") \
            .search(process_output) \
            .group(1)

        with open(f"{self.raw_dir}/jobs-{config.repetition}", "a") as f:
            f.write(job_id + "\n")

        logger.info(f'submitted job {job_id}')

    def verify(self, repetition: int) -> bool:
        completed = True
        with open(f"{self.raw_dir}/jobs-{repetition}") as f:
            for job_id in f.read().splitlines():
                if os.path.isfile(f'{self.raw_dir}/{job_id}'):
                    continue

                proc = subprocess.run(
                    [
                        "bjobs",
                        "-o", "jobid stat exit_code",
                        "-json",
                        job_id,
                    ],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                msg = json.loads(proc.stdout.decode())
                status = msg['RECORDS'][0]['STAT']
                logger.info(f"{job_id} is {status}")
                if status != "DONE":
                    completed = False

        return completed

    @staticmethod
    def find_key(job_id, data: list, key: str) -> str:
        line = [x for x in data if key in x]
        if len(line) != 1:
            raise Exception(f'[{job_id}] expected length 1 for key {key}, got {len(line)}')

        return line[0].split(":")[1].split()

    @staticmethod
    def collect_lines(data: typing.List[str]) -> typing.List[typing.Tuple[int, str]]:
        start, end = (0, 0)
        for idx, line in enumerate(data):
            if line == "The output (if any) follows:\n":
                start = idx
            if line == "PS:\n":
                end = idx

        # account for padding
        start += 2
        end -= 2

        line_nrs = range(start + 1, end + 1)  # line numbers are usually 1-indexed
        return list(zip(line_nrs, data[start:end]))

    def find_mem_max(self, job_id: str, data: list) -> float:
        max_mem = self.find_key(job_id, data, "Max Memory")[0]
        if max_mem == '-':
            return 0

        return float(max_mem)

    def collect(self, repetition: int):
        with open(f"{self.parsed_dir}/{repetition}.json", "w") as o:
            with open(f"{self.raw_dir}/jobs-{repetition}") as f:
                for job_id in f.read().splitlines():
                    input_path = f'{self.raw_dir}/{job_id}'
                    try:
                        with open(input_path) as j:
                            data = j.readlines()

                            data_lines = self.collect_lines(data)
                            job_data = {
                                'id': job_id,
                                'turnaround_time': int(self.find_key(job_id, data, "Turnaround time")[0]),
                                'runtime': int(self.find_key(job_id, data, "Run time")[0]),
                                'mem_requested': float(self.find_key(job_id, data, "Total Requested Memory")[0]),
                                'mem_max': self.find_mem_max(job_id, data),
                            }

                            subject = data[1].strip()
                            if len(data_lines) == 0:
                                logging.error(f'[{job_id}] no usable data lines found, {subject}')

                            for line_nr, line in data_lines:
                                try:
                                    parsed = json.loads(line)
                                    parsed['job'] = job_data
                                    o.write(json.dumps(parsed, separators=(',', ':')) + '\n')
                                except Exception as e:
                                    logging.error(f'[{job_id}] failed to parse line {line_nr}, "{subject}": {e}')
                                    break
                    except FileNotFoundError:
                        logging.error(f'[{job_id} no job output file found (yet): {input_path}')
