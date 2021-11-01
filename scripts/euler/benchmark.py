import os
import argparse
from lib import *


def inclusive(min_val: int, max_val: int, step=1):
    return range(min_val, max_val + 1, step)


vector_sizes = [
    Configuration(n=2 ** n, m=2 ** n, nodes=2 ** p, repetition=r, implementation=implementation)
    for n in inclusive(4, 13)
    for p in inclusive(2, 5)
    for r in repetitions
    for implementation in ["allgather", "allreduce"]
]

nodes = [
    Configuration(n=2 ** 13, m=2 ** 13, nodes=p, repetition=r, implementation=implementation)
    for p in inclusive(2, 48, 2)
    for r in repetitions
    for implementation in ["allgather", "allreduce"]
]


def is_euler() -> bool:
    return 'HOSTNAME' in os.environ and os.environ['HOSTNAME'].startswith('eu')


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-d', '--dry-run', action="store_true", default=False)
    args = parser.parse_args()
    dry_run = args.dry_run

    if dry_run:
        scheduler = Scheduler(DryRun())
    else:
        scheduler = Scheduler(EulerRunner())

    scheduler.register(config=vector_sizes)
    scheduler.register(config=nodes)
    scheduler.run()
    if dry_run:
        print(f"{len(scheduler.configs)} configurations")


if __name__ == '__main__':
    main()
