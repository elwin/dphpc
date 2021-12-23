#!/usr/bin/env python3

import glob
import argparse
import pathlib
import os

from scheduler import EulerRunner
from config import results_path

import plot

folder_offsets = {  # sorry but i have no easier Idea to collect them easily
    "mixed/dave": 0 * 17,
    "mixed/dave2": (4 * 17) + (0 * 15),
    "mixed/elwin": 1 * 17,
    "mixed/elwin2": (4 * 17) + (1 * 15),
    "mixed/noe": 2 * 17,
    "mixed/noe2": (4 * 17) + (2 * 15),
    "mixed/patrick": 3 * 17,
    "mixed/patrick2": (4 * 17) + (3 * 15),
    "mixed/roysc": 0,
    "mixed/roysc2": (4 * 17),
    "raw": 0,
}


def main():
    parser = argparse.ArgumentParser(description='Collect all raw benchmark files')
    parser.add_argument('action', choices=['all', 'collect', 'plot'], default='all', nargs='?')
    parser.add_argument('-a',
                        '--aggregate',
                        type=str,
                        default="raw",
                        help="Directory unaggregated results from multiple benchmarks")
    parser.add_argument('-d',
                        '--dir',
                        type=str,
                        default=results_path,
                        help="Directory containing results (both in and output")
    args = parser.parse_args()

    if args.action in ['all', 'collect']:
        folders = ['raw']
        if args.aggregate != "raw":
            subfolders = os.listdir(f"{args.dir}/{args.aggregate}")
            folders = [f"{args.aggregate}/{f}" for f in subfolders]

        # Delete all current aggregated files
        try:
            parsed_dir = f"{args.dir}/parsed/"
            files = [f for f in os.listdir(parsed_dir)]
            for file in files:
                os.remove(f"{parsed_dir}{file}")
        except OSError as e:
            print(f"Error when deleting aggregated files: {e}")

        # jesus what have I done...
        for folder in folders:
            if folder.startswith('.'):
                continue
            for file in glob.glob(f'{args.dir}/{folder}/jobs-*'):
                filename = pathlib.Path(file).name
                repetition = int(filename.strip('jobs-'))
                EulerRunner(results_dir=args.dir, raw_dir=folder).collect(repetition,
                                                                          repetition_offset=folder_offsets[folder])

    if args.action in ['all', 'plot']:
        input_dir = f"{args.dir}/parsed"
        input_files = glob.glob(f'{args.dir}/parsed/*.json')
        output_dir = f'{args.dir}/plots'
        pathlib.Path(output_dir).mkdir(exist_ok=True)
        plot.plot(input_files=input_files, input_dir=input_dir, output_dir=output_dir)


if __name__ == '__main__':
    main()
