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


def drop(li: list, key: str) -> list:
    li = li.copy()
    li.remove(key)
    return li


@dataclasses.dataclass(eq=True, frozen=True, order=True)
class Configuration:
    n: int
    m: int
    nodes: int
    implementation: str
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
            '-i', self.implementation,
        ]
        if self.verify:
            args.append('-c')

        return args


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
        if config.nodes > 48:
            logging.warning(f"Euler may support only up to 48 nodes, {config.nodes} requested")

        if config.n > 2 ** 13 or config.m > 2 ** 13:
            logging.warning(f"vectors larger than {2 ** 13} entries may fail on euler due to memory constraints")


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
                                'mem_max': float(self.find_key(job_id, data, "Max Memory")[0]),
                            }

                            if len(data_lines) == 0:
                                logging.error(f'[{job_id}] no usable data lines found')

                            for line_nr, line in data_lines:
                                try:
                                    parsed = json.loads(line)
                                    parsed['job'] = job_data
                                    o.write(json.dumps(parsed, separators=(',', ':')) + '\n')
                                except Exception as e:
                                    subject = data[1]
                                    logging.error(f'[{job_id}] failed to parse line {line_nr}, "{subject}": {e}')
                                    break
                    except FileNotFoundError:
                        logging.error(f'[{job_id} no job output file found (yet): {input_path}')
