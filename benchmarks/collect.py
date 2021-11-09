import glob

from benchmark import *


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-d', '--dir', type=str, default=results_path,
                        help="Directory containing results (both in and output")
    args = parser.parse_args()

    # jesus what have I done...
    for file in glob.glob(f'{args.dir}/raw/jobs-*'):
        filename = pathlib.Path(file).name
        repetition = int(filename.strip('jobs-'))
        EulerRunner(results_dir=args.dir).collect(repetition)


if __name__ == '__main__':
    main()
