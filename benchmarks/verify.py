#!/usr/bin/env python3

from benchmark import *


def main():
    if False in [EulerRunner(results_dir=results_path).verify(r) for r in repetitions]:
        exit(1)


if __name__ == '__main__':
    main()
