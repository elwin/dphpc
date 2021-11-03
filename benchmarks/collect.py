from benchmark import *


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-d', '--dir', type=str, default=results_path,
                        help="Directory containing results (both in and output")
    args = parser.parse_args()

    [EulerRunner(results_dir=args.dir).collect(r) for r in repetitions]


if __name__ == '__main__':
    main()