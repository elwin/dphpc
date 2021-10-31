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


class DryRun(Runner):
    def run(self, config: Configuration):
        print(f"{config.n} {config.m} {config.nodes} {config.implementation}")


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


class EulerRunner(Runner):
    def run(self, config: Configuration):
        proc = subprocess.run(
            [
                "bsub",
                "-o", "bb/%J",
                "-e", "bb/%J.err",
                "-n", str(config.nodes),
                "mpirun",
                "-np", str(config.nodes),
                binary_path,
                "-n", str(config.n),
                "-m", str(config.m),
                "-i", config.implementation,
            ],
            stdout=subprocess.PIPE,
        )

        job = re \
            .compile("Job <(.*)> is submitted to queue <normal.4h>.") \
            .search(proc.stdout.decode()) \
            .group(1)

        with open(f"bb/jobs-{config.repetition}", "a") as f:
            f.write(job + "\n")

    def collect(self, repetition: int):
        with open(f"out/{repetition}.json", "a") as o:
            with open(f"bb/jobs-{repetition}") as f:
                for job_id in f.read().splitlines():
                    try:
                        with open(f"bb/{job_id}") as j:
                            o.writelines(j.readlines()[35:][:-6])
                    except:
                        pass
