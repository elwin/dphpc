import collections.abc
import dataclasses
import enum
import io
import itertools
import json
import logging
import os.path
import pathlib
import re
import subprocess
import sys
import tempfile
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
    allgather_algorithm: int = None

    def __str__(self):
        return self.name


allgather = Implementation(name='allgather')
allreduce = Implementation(name='allreduce')
allreduce_ring = Implementation(name='allreduce-ring')
allreduce_butterfly = Implementation(name='allreduce-butterfly')
allgather_async = Implementation(name='allgather-async')
allreduce_rabenseifner = Implementation(name='allreduce-rabenseifner')
rabenseifner_gather = Implementation(name='rabenseifner-gather')
grabenseifner_allgather = Implementation(name='g-rabenseifner-allgather')
bruck_async = Implementation(name='bruck-async')

native_allreduce = [
    Implementation(name='allreduce-native-basic_linear', allreduce_algorithm=1),
    Implementation(name='allreduce-native-nonoverlapping', allreduce_algorithm=2),
    Implementation(name='allreduce-native-recursive_doubling', allreduce_algorithm=3),
    Implementation(name='allreduce-native-ring', allreduce_algorithm=4),
    Implementation(name='allreduce-native-segmented_ring', allreduce_algorithm=5),
    Implementation(name='allreduce-native-rabenseifner', allreduce_algorithm=6),
]

native_allgather = [
    Implementation(name='allgather-native-linear', allgather_algorithm=1),
    Implementation(name='allgather-native-bruck', allgather_algorithm=2),
    Implementation(name='allgather-native-recursive_doubling', allgather_algorithm=3),
    Implementation(name='allgather-native-ring', allgather_algorithm=4),
    Implementation(name='allgather-native-neighbor', allgather_algorithm=5),
    Implementation(name='allgather-native-sparbit', allgather_algorithm=6),
]


@dataclasses.dataclass(eq=True, frozen=True, order=True)
class Configuration:
    n: int
    m: int
    nodes: int
    implementation: Implementation
    repetitions: int = 1  # used for repetitions within a job (-t ${repetitions})
    job_repetition: int = 0  # used for repeated jobs
    verify: bool = False

    def __str__(self):
        dim = f'{self.n}' if self.n == self.m else f'{self.n}x{self.m}'
        out = f'{dim}, {self.nodes} nodes, {self.implementation}, {self.repetitions}x'
        if self.verify:
            out += ' [verification]'

        return out

    def command(self):
        args = [
            f'./{binary_path}',  # Make sure we use a proper path
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
        raise NotImplemented

    def run_grouped(self, keys, configs: typing.List[Configuration]):
        raise NotImplemented

    def collect(self, repetition: int):
        raise NotImplemented

    def verify(self, repetition: int) -> bool:
        raise NotImplemented


class Scheduler:
    def __init__(self, runner: Runner):
        self.configs = []
        self.runner = runner

    def register(self, config):
        if isinstance(config, collections.abc.Iterable):
            self.configs.extend(config)
        else:
            self.configs.append(config)

    @staticmethod
    def grouping_key(c: Configuration):
        return c.nodes, c.job_repetition

    def run_grouped(self):
        grouped = itertools.groupby(
            sorted(self.valid_configurations(), key=self.grouping_key),
            self.grouping_key,
        )

        for nodes, configs in grouped:
            self.runner.run_grouped(nodes, list(configs))

    def run(self):
        for config in self.configurations():
            self.runner.run(config)

    def configurations(self):
        configs = list(set(self.configs))
        configs.sort()
        return configs

    def valid_configurations(self):
        configs = list(set(self.configs))
        configs = list(filter(lambda c: c.runnable(), configs))
        configs.sort()
        return configs


class DryRun(Runner):
    def run(self, config: Configuration):
        logger.info(str(config))

        runnable, reason = config.runnable()
        if not runnable:
            logging.warning(reason)

    def run_grouped(self, keys, configs: typing.List[Configuration]):
        logging.info(f'{keys} -> {len(configs)} configurations')


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

    def actually_run(self, nodes: int, job_repetition: int, mpi_args: str, time: int = None):
        args = [
            'bsub',
            # '-J', f'"{config.__str__()}"',
            '-o', f'{self.raw_dir}/%J',
            '-e', f'{self.raw_dir}/%J.err',
            '-n', str(nodes),
            '-R', 'span[ptile=1]',  # use 1 core per node
            '-R', 'select[model==XeonE3_1585Lv5]',  # use Euler III nodes (4 cores)
            '-R', 'select[!ib]',  # disable infiniband (not available on Euler III)
            '-r',  # make jobs retryable
        ]
        if time is not None and time > 4 * 60:
            args.extend(['-W', str(time)])

        args.append(mpi_args)

        logger.debug("executing the following command:")
        logger.info(" ".join(args))

        # proc = subprocess.run(args, stdout=subprocess.PIPE)
        # process_output = proc.stdout.decode()
        # logger.debug(process_output)
        #
        # job_id = re \
        #     .compile("Job <(.*)> is submitted to queue.") \
        #     .search(process_output) \
        #     .group(1)
        #
        # with open(f"{self.raw_dir}/jobs-{job_repetition}", "a") as f:
        #     f.write(job_id + "\n")

        # logger.info(f'submitted job {job_id}')

    @staticmethod
    def prepare_cmd(config: Configuration):
        mpi_args = [
            'mpirun',
            '-np', str(config.nodes),
        ]
        if config.implementation.allreduce_algorithm is not None:
            mpi_args.extend([
                '--mca', 'coll_tuned_use_dynamic_rules', '1',
                '--mca', 'coll_tuned_allreduce_algorithm', f'{config.implementation.allreduce_algorithm}'
            ])

        if config.implementation.allgather_algorithm is not None:
            mpi_args.extend([
                '--mca', 'coll_tuned_use_dynamic_rules', '1',
                '--mca', 'coll_tuned_allgather_algorithm', f'{config.implementation.allgather_algorithm}'
            ])

        if config.implementation.allreduce_algorithm is not None \
                and config.implementation.allgather_algorithm is not None:
            raise Exception("can only specify one of {allreduce_algorithm, allgather_algorithm), not both")

        mpi_args.extend(config.command())

        return mpi_args

    def run(self, config: Configuration):
        runnable, reason = config.runnable()
        if not runnable:
            logger.warning(f'skipping configuration: {reason}')
            return

        mpi_args = self.prepare_cmd(config)

        self.actually_run(config.nodes, config.job_repetition, ' '.join(mpi_args))

    def run_grouped(self, keys, configs: typing.List[Configuration]):
        nodes = configs[0].nodes
        repetition = configs[0].job_repetition

        commands = []
        for config in configs:
            runnable, reason = config.runnable()
            if not runnable:
                logger.warning(f'skipping configuration: {reason}')
                continue

            if config.nodes != nodes:
                raise Exception(
                    f'different number of nodes in same grouping, expected {nodes}, received {config.nodes}')

            if config.job_repetition != repetition:
                raise Exception(
                    f'different job repetition in same grouping, expected {repetition}, received {config.job_repetition}')

            mpi_args = self.prepare_cmd(config)
            commands.append(' '.join(mpi_args))

        time = 2 * len(configs)  # roughly 1.25 minutes / run on average

        with tempfile.NamedTemporaryFile(delete=False, mode='wt') as f:
            for line in commands:
                f.write(f'{line}\n')

            self.actually_run(nodes, repetition, f'< {f.name}', time)

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
