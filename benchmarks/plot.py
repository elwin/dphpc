import dataclasses
import json
import typing

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import pandas.io.json

agg_func = np.median


@dataclasses.dataclass
class PlotManager:
    output_dir: str

    def plot_runtime(self, df: pd.DataFrame):
        for i in [2 ** i for i in range(1, 6)]:
            data = df[df['numprocs'] == i]

            data.pivot_table(
                index='N',
                columns='implementation',
                values='runtime',
                aggfunc=agg_func,
            ).plot(
                title=f'Runtime ({i} nodes)',
                kind='line',
                logy=True,
                ylabel='Runtime (s)',
                xlabel='Input Dimension',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/runtime_{i}.svg')
            plt.close()

        for n in [2 ** n for n in range(4, 14)]:
            data = df[df['N'] == n]

            data.pivot_table(
                index='numprocs',
                columns='implementation',
                values='runtime',
                aggfunc=agg_func,
            ).plot(
                title=f'Runtime ({n} vector size)',
                kind='line',
                logy=True,
                ylabel='Runtime (s)',
                xlabel='Number of processes',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/runtime_dim_{n}.svg')
            plt.close()

    def plot_mem_usage(self, df: pd.DataFrame):
        for i in [2 ** i for i in range(1, 6)]:
            data = df[df['numprocs'] == i]
            data = data[['N', 'implementation', 'job.mem_max_avg', 'job.mem_requested']]

            data.pivot_table(
                index='N',
                columns='implementation',
                values='job.mem_max_avg',
                aggfunc=agg_func,
            ).plot(
                title=f'Memory Usage ({i} nodes)',
                kind='bar',
                ylabel='Memory Usage (GB)',
                xlabel='Input Dimension',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/mem_usage_{i}.svg')
            plt.close()

    def plot_compute_ratio(self, df: pd.DataFrame):
        for i in [2 ** i for i in range(1, 6)]:
            data = df[df['numprocs'] == i]

            data.pivot_table(
                index='N',
                columns='implementation',
                values='fraction',
                aggfunc=agg_func,
            ).plot(
                title=f'Compute ratio ({i} nodes)',
                kind='bar',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/compute_ratio_{i}.svg')
            plt.close()

    def plot_repetitions(self, df: pd.DataFrame):
        for num_procs in [2 ** i for i in range(1, 6)]:
            vector_size = 4096

            data = df[(df['M'] == vector_size) & (df['numprocs'] == num_procs)]

            data.pivot_table(
                index='iteration',
                columns='implementation',
                values='runtime',
                aggfunc=agg_func,
            ).plot(
                title=f'Runtime across repetition number ({vector_size} vector size, {num_procs} nodes)',
                ylabel='Runtime (s)',
                xlabel='Iteration',
                kind='bar',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/runtime_repetition_{num_procs}.svg')
            plt.close()


def plot(input_files: typing.List[str], output_dir: str):
    pm = PlotManager(output_dir=output_dir)

    dfs = []
    for input_file in input_files:
        df = pandas.io.json.read_json(input_file, lines=True)
        df = pd.json_normalize(json.loads(df.to_json(orient="records")))  # flatten nested json
        dfs.append(df)
    df = pd.concat(dfs)

    df = df.rename(columns={'name': 'implementation'})
    df['job.mem_max'] /= 1000
    df['job.mem_max_avg'] = df['job.mem_max'] / df['numprocs']
    df['runtime_compute'] /= 1_000_000
    df['runtime'] /= 1_000_000
    df['fraction'] = df['runtime_compute'] / df['runtime']
    df['iteration'] += 1

    pm.plot_runtime(df)
    pm.plot_compute_ratio(df)
    pm.plot_mem_usage(df)
    pm.plot_repetitions(df)
