import os

from lib import *

configs = [
    Configuration(n=2 ** n, m=2 ** n, nodes=2 ** p, repetition=r, implementation=implementation)
    for n in range(4, 17)
    for p in range(2, 6)
    for r in repetitions
    for implementation in ["allgather", "allreduce"]
]

def is_euler() -> bool:
    return 'HOSTNAME' in os.environ and os.environ['HOSTNAME'].startswith('eu')


def main():
    if is_euler():
        scheduler = Scheduler(EulerRunner())
    else:
        scheduler = Scheduler(DryRun())

    scheduler.register(config=configs)
    scheduler.run()


if __name__ == '__main__':
    main()
