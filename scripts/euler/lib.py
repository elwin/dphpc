import enum
import json
import logging
import subprocess
import re
import io
import sys
from collections import abc as collections
from config import *

logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger()


class Configuration:
    def __init__(self, n: int, m: int, nodes: int, implementation: str, repetition: int):
        self.n = n
        self.m = m
        self.nodes = nodes
        self.implementation = implementation
        self.repetition = repetition

    def __str__(self):
        dim = f'{self.n}' if self.n == self.m else f'{self.n}x{self.m}'
        return f'{dim}, {self.nodes} nodes, {self.implementation}, rep {self.repetition}'


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
        if isinstance(config, collections.Iterable):
            self.configs.extend(config)
        else:
            self.configs.append(config)

    def run(self):
        for config in self.configs:
            self.runner.run(config)

    def configurations(self):
        return self.configs


class DryRun(Runner):
    def run(self, config: Configuration):
        logger.info(f"{config.n} x {config.m}, {config.nodes} nodes, {config.implementation}, rep {config.repetition}")
        if config.nodes > 48:
            logging.warning(f"Euler may support only up to 48 nodes, {config.nodes} requested")

        if config.n > 2 ** 13 or config.m > 2 ** 13:
            logging.warning(f"vectors larger than {2 ** 13} entries may fail on euler due to memory constraints")


class LocalRunner(Runner):
    def __init__(self, output: io.TextIOWrapper):
        self.output = output

    def run(self, config: Configuration):
        proc = subprocess.run(
            [
                binary_path,
                "-n", str(config.n),
                "-m", str(config.m),
                "-i", config.implementation,
            ],
            stdout=subprocess.PIPE,
        )

        self.output.write(proc.stdout.decode())


class Status(enum.Enum):
    DONE = 1
    PENDING = 2
    RUNNING = 3
    FAILED = 4


class EulerRunner(Runner):
    def run(self, config: Configuration):
        proc = subprocess.run(
            [
                "bsub",
                "-J", config.__str__(),
                "-o", f"{bb_output_path}/%J",
                "-e", f"{bb_output_path}/%J.err",
                "-n", str(config.nodes),
                "-R", "span[ptile=1] select[model==XeonE3_1585Lv5]",  # use 1 core per node on Euler III nodes (4 cores)
                "-r",  # make jobs retryable
                "mpirun",
                "-np", str(config.nodes),
                binary_path,
                "-n", str(config.n),
                "-m", str(config.m),
                "-i", config.implementation,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )

        logger.info(proc.stdout.decode())

        job_id = re \
            .compile("Job <(.*)> is submitted to queue <normal.4h>.") \
            .search(proc.stdout.decode()) \
            .group(1)

        with open(f"{bb_output_path}/jobs-{config.repetition}", "a") as f:
            f.write(job_id + "\n")

        logger.info(f'submitted job {job_id}')

    def verify(self, repetition: int) -> bool:
        completed = True
        with open(f"{bb_output_path}/jobs-{repetition}") as f:
            for job_id in f.read().splitlines():
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
                status = {
                    "DONE": "done",
                    "PEND": "pending",
                    "EXIT": "failed",
                    "RUN": "running"
                }[msg['RECORDS'][0]['STAT']]
                logger.info(f"{job_id} is {status}")
                if status != "done":
                    completed = False

        return completed

    def collect(self, repetition: int):
        with open(f"{parsed_output_path}/{repetition}.json", "w") as o:
            with open(f"{bb_output_path}/jobs-{repetition}") as f:
                for job_id in f.read().splitlines():
                    try:
                        with open(f"{bb_output_path}/{job_id}") as j:
                            data = [x for x in j.readlines() if x.startswith("{")]  # poor man grep
                            if len(data) != 1:
                                logger.info(f'[{job_id}] expected length 1, got {len(data)}')
                                continue

                            parsed = json.loads(data[0])
                            if 'timestamp' not in parsed:
                                logging.error(f'[{job_id}] no timestamp found in output')

                            o.writelines(data)
                    except:
                        logging.error(f'[{job_id}] failed to read output')
