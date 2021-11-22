#!/usr/bin/env python3

import argparse
import shutil

from scheduler import *

implementations = [
    allgather,
    allreduce,
    allreduce_ring,
    allreduce_butterfly,
    allgather_async,
    bruck_async,
    allreduce_rabenseifner,
    rabenseifner_gather,
    # rabenseifner_scatter, # not ready yet

    # Native implementations
    allreduce_native_ring,
    allreduce_native_basic_linear,
    allreduce_native_rabenseifner,
    allreduce_native_nonoverlapping,
    allreduce_native_recursive_doubling,
    allreduce_native_segmented_ring,
]
repetitions = 40

configs = []
configs.extend([
    Configuration(n=2 ** n, m=2 ** n, nodes=2 ** p, repetitions=repetitions, implementation=implementation)
    for n in inclusive(4, 13)
    for p in inclusive(1, 5)
    for implementation in implementations
])

configs.extend([
    Configuration(n=2 ** n, m=2 ** n, nodes=p, repetitions=repetitions, implementation=implementation)
    for p in inclusive(2, 48, 2)
    for n in inclusive(4, 13)
    for implementation in drop(implementations, [
        allreduce_rabenseifner,
        allreduce_butterfly,
        rabenseifner_gather,
    ])
])

verify_configs = [
    Configuration(n=2 ** n, m=2 ** n, nodes=2 ** p, implementation=implementation, verify=True)
    for n in inclusive(4, 10)
    for p in inclusive(2, 5)
    for implementation in implementations
]


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-m', '--mode', type=str, default="dry-run", help="One of {dry-run, euler}")
    parser.add_argument('-c', '--clean', action="store_true", default=False, help="Clean results directory first")
    parser.add_argument('--check', '--verify', action="store_true", default=False, help="Check results for correctness")
    args = parser.parse_args()

    if args.clean:
        logger.info(f"cleaning results directory ({results_path})")
        shutil.rmtree(results_path)

    mode = args.mode
    if mode == "dry-run":
        scheduler = Scheduler(DryRun())
    elif mode == "euler":
        scheduler = Scheduler(EulerRunner(results_dir=results_path))
    else:
        parser.print_help()
        return

    scheduler.register(config=configs if not args.check else verify_configs)
    scheduler.run()

    if mode == "dry-run":
        valid_configs = filter(lambda config: config.runnable()[0], scheduler.configurations())

        logger.info(f"{len(list(valid_configs))} configurations")


if __name__ == '__main__':
    main()
