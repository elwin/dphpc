import argparse
import shutil

from scheduler import *

implementations = ["allgather", "allreduce"]

configs = []
configs.extend([
    Configuration(n=2 ** n, m=2 ** n, nodes=4, repetition=r, implementation=implementation)
    # for n in inclusive(4, 13)
    for n in inclusive(4, 8)
    # for p in inclusive(2, 5)
    for r in repetitions
    for implementation in implementations
])

configs.extend([
    # Configuration(n=2 ** 13, m=2 ** 13, nodes=p, repetition=r, implementation=implementation)
    # for p in inclusive(2, 48, 2)
    # for r in repetitions
    # for implementation in implementations
])


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-m', '--mode', type=str, default="dry-run", help="One of {dry-run, euler}")
    parser.add_argument('-c', '--clean', action="store_true", default=False, help="Clean results directory first")
    args = parser.parse_args()

    if args.clean:
        logger.info(f"cleaning results directory ({results_path})")
        shutil.rmtree(results_path)

    mode = args.mode
    if mode == "dry-run":
        scheduler = Scheduler(DryRun())
    elif mode == "euler":
        scheduler = Scheduler(EulerRunner(
            raw_dir=f"{results_path}/raw",
            parsed_dir=f"{results_path}/parsed",
        ))
    else:
        parser.print_help()
        return

    scheduler.register(config=configs)
    scheduler.run()

    if mode == "dry-run":
        logger.info(f"{len(scheduler.configs)} configurations")


if __name__ == '__main__':
    main()
