from benchmark import *


def main():
    [EulerRunner(results_dir=results_path).collect(r) for r in repetitions]


if __name__ == '__main__':
    main()
