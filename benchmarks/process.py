#!/usr/bin/env python3

import glob

from benchmark import *
import plot


def main():
    parser = argparse.ArgumentParser(description='Collect all raw benchmark files')
    parser.add_argument('action', choices=['all', 'collect', 'plot'], default='all', nargs='?')
    parser.add_argument('-d', '--dir', type=str, default=results_path,
                        help="Directory containing results (both in and output")
    args = parser.parse_args()

    if args.action in ['all', 'collect']:
        # jesus what have I done...
        for file in glob.glob(f'{args.dir}/raw/jobs-*'):
            filename = pathlib.Path(file).name
            repetition = int(filename.strip('jobs-'))
            EulerRunner(results_dir=args.dir).collect(repetition)

    if args.action in ['all', 'plot']:
        input_files = glob.glob(f'{args.dir}/parsed/*.json')
        output_dir = f'{args.dir}/plots'
        pathlib.Path(output_dir).mkdir(exist_ok=True)
        plot.plot(input_files=input_files, output_dir=output_dir)


if __name__ == '__main__':
    main()
