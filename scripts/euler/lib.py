import enum
import json

from config import *
from collections import abc as collections
import subprocess
import re
import io


class Configuration:
    def __init__(self, n: int, m: int, nodes: int, implementation: str, repetition: int):
        self.n = n
        self.m = m
        self.nodes = nodes
        self.implementation = implementation
        self.repetition = repetition


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
        print(f"{config.n} x {config.m}, {config.nodes} nodes, {config.implementation}, rep {config.repetition}")


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
                "-o", f"{bb_output_path}/%J",
                "-e", f"{bb_output_path}/%J.err",
                "-n", str(config.nodes),
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

        job_id = re \
            .compile("Job <(.*)> is submitted to queue <normal.4h>.") \
            .search(proc.stdout.decode()) \
            .group(1)

        with open(f"{bb_output_path}/jobs-{config.repetition}", "a") as f:
            f.write(job_id + "\n")

        print(f'submitted job {job_id}')

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
                print(f"{job_id} is {status}")
                if status != "done":
                    completed = False

        return completed

    def collect(self, repetition: int):
        with open(f"{parsed_output_path}/{repetition}.json", "w") as o:
            with open(f"{bb_output_path}/jobs-{repetition}") as f:
                for job_id in f.read().splitlines():
                    try:
                        with open(f"{bb_output_path}/{job_id}") as j:
                            o.writelines([job_id])
                            data = j.readlines()[35:][:-6]
                            if len(data) != 1:
                                print(f'expected length 1, got {len(data)}')
                                continue

                            parsed = json.loads(data[0])
                            if 'timestamp' not in parsed:
                                print(f'no timestamp found in output')

                            o.writelines(data)
                    except:
                        print(f'failed to read output for job {job_id}')
