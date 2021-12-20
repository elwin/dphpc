#!/usr/bin/env python3

import argparse
import shutil

from scheduler import *

implementations = [
    allgather,
    allreduce,
    allreduce_ring,
    # allreduce_butterfly,
    # allgather_async,
    # bruck_async,
    # allreduce_rabenseifner,
    # rabenseifner_gather,
    # grabenseifner_allgather,
    #
    # *native_allreduce,
    # *native_allgather,

    grabenseifner_allgather,
    # grabenseifner_subgroup_1,
    grabenseifner_subgroup_2,
    grabenseifner_subgroup_4,
    grabenseifner_subgroup_8,
    # grabenseifner_subgroup_16
]

repetitions = 25
job_repetitions = 15

configs = []
configs.extend([
    Configuration(
        n=n,
        m=n,
        nodes=nodes,
        repetitions=repetitions,
        job_repetition=job_repetition,
        implementation=implementation,
    )
    for n in inclusive(1000, 8000, 1000)
    for nodes in [32, 48]
    for job_repetition in range(job_repetitions)
    for implementation in implementations
])

verify_configs = [
    Configuration(n=2 ** n, m=2 ** n, nodes=2 ** p, implementation=implementation, verify=True)
    for n in inclusive(4, 10)
    for p in inclusive(2, 5)
    for implementation in implementations
]


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-m', '--mode', type=str, default="dry-run", help="One of {dry-run, euler, euler-files}")
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
    elif mode == "euler-files":
        scheduler = Scheduler(EulerRunner(results_dir=results_path, submit=False))
    else:
        parser.print_help()
        return

    scheduler.register(config=configs if not args.check else verify_configs)

    scheduler.run_grouped()

    if mode == "dry-run":
        valid_configs = filter(lambda config: config.runnable()[0], scheduler.configurations())

        logger.info(f"{len(list(valid_configs))} configurations")


if __name__ == '__main__':
    main()
