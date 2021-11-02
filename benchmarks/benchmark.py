import argparse
from shared import *

implementations = ["allgather", "allreduce"]

configs = []
configs.extend([
    Configuration(n=2 ** n, m=2 ** n, nodes=8, repetition=r, implementation=implementation)
    for n in inclusive(4, 13)
    for p in inclusive(2, 5)
    for r in repetitions
    for implementation in implementations
])

configs.extend([
    Configuration(n=2 ** 13, m=2 ** 13, nodes=p, repetition=r, implementation=implementation)
    for p in inclusive(2, 48, 2)
    for r in repetitions
    for implementation in implementations
])


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-m', '--mode', type=str, default="dry-run", help="one of {dry-run, euler}")
    args = parser.parse_args()
    mode = args.mode

    if mode == "dry-run":
        scheduler = Scheduler(DryRun())
    elif mode == "euler":
        scheduler = Scheduler(EulerRunner())
    else:
        parser.print_help()
        return

    scheduler.register(config=configs)
    scheduler.run()

    if mode == "dry-run":
        logger.info(f"{len(scheduler.configs)} configurations")


if __name__ == '__main__':
    main()
