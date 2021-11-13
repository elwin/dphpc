import dataclasses
import json
import typing

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import pandas.io.json
import math

agg_func = np.median


@dataclasses.dataclass
class PlotManager:
    output_dir: str

    def plot_for_analysis(self, df: pd.DataFrame, func_key='median'):
        self.plot_runtime_with_scatter(df, 'numprocs', 'N', func_key=func_key)
        self.plot_runtime_with_scatter(df, 'N', 'numprocs', func_key=func_key)
        self.plot_runtime_by_key(df, 'N', 'numprocs', func_key=func_key)
        self.plot_runtime_by_key(df, 'numprocs', 'N', func_key=func_key)

    def plot_runtime_by_key(self, df: pd.DataFrame, filter_key='N', index_key='numprocs', func_key='mean',
                            powers_of_two=True):
        if func_key == 'mean':
            agg_func = np.mean
        elif func_key == 'median':
            agg_func = np.median
        elif func_key == '99':
            agg_func = self.ninety_nine
        else:
            agg_func = np.mean

        # print(df[filter_key].unique())
        # print(df)
        for i in df[filter_key].unique():
            # print(math.log(i, 2))
            if powers_of_two and not math.log(i, 2).is_integer():
                continue

            # print("entered")
            data = df[df[filter_key] == i]
            data.pivot_table(
                index=index_key,
                columns='implementation',
                values='runtime',
                aggfunc=agg_func
            ).plot(
                title=f'Runtime ({i} nodes)',
                kind='line',
                logy=True,
                ylabel='Runtime (s)',
                xlabel='Input Dimension',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/runtime_{i}_{filter_key}_{func_key}.svg')
            plt.close()

    def plot_runtime_with_scatter(self, df: pd.DataFrame, filter_key='N', index_key='numprocs', func_key='median',
                                  powers_of_two=True):
        if func_key == 'mean':
            agg_func = np.mean
        elif func_key == 'median':
            agg_func = np.median
        elif func_key == '99':
            agg_func = self.ninety_nine
        else:
            agg_func = np.mean

        for i in df[filter_key].unique():
            if powers_of_two and not math.log(i, 2).is_integer():
                continue
            data = df[df[filter_key] == i]

            color_dict = self.map_colors(data['implementation'].unique())
            fig, ax = plt.subplots()
            ax.scatter(data[index_key], data['runtime'], color=[color_dict.get(x) for x in data['implementation']])
            data = df[df[filter_key] == i]
            data_filtered = data.pivot_table(
                index=index_key,
                columns='implementation',
                values='runtime',
                aggfunc=agg_func
            )

            for col in data_filtered.columns:
                # pprint(data_filtered.index)
                # pprint(data_filtered[col])
                x, y = self.remove_nan_from_cols(data_filtered.index, data_filtered[col])
                ax.plot(x, y, color=color_dict.get(col), label=col)
            ax.set_yscale('log')
            plt.tight_layout()
            plt.legend(loc="upper left")
            plt.savefig(f'{self.output_dir}/runtime_{i}_{filter_key}_{func_key}_with_scatter.svg')
            plt.close()

    @staticmethod
    def map_colors(implementation_types):
        colors = ['red', 'blue', 'green', 'purple', 'yellow', 'pink', 'gray', 'orange']
        color_dict = {}
        for i in range(len(implementation_types)):
            color_dict[implementation_types[i]] = colors[i]
        return color_dict

    @staticmethod
    def remove_nan_from_cols(index, col):
        col_arr = col.to_numpy()
        x = []
        y = []
        for i in range(len(index)):
            if not np.isnan(col_arr[i]):
                x.append(index[i])
                y.append(col_arr[i])
        return x, y

    @staticmethod
    def ninety_nine(p):
        return np.percentile(p, 99)

    def plot_runtime(self, df: pd.DataFrame):
        for i in [2 ** i for i in range(1, 6)]:
            data = df[df['numprocs'] == i]
            if data.shape[0] == 0:
                continue

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
            if data.shape[0] == 0:
                continue

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
            if data.shape[0] == 0:
                continue

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
            if data.shape[0] == 0:
                continue

            data.pivot_table(
                index='N',
                columns='implementation',
                values='fraction',
                aggfunc=agg_func,
            ).plot(
                title=f'Compute ratio ({i} nodes)',
                kind='line',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/compute_ratio_{i}.svg')
            plt.close()

        for n in [2 ** n for n in range(4, 14)]:
            data = df[df['N'] == n]
            if data.shape[0] == 0:
                continue

            data.pivot_table(
                index='numprocs',
                columns='implementation',
                values='fraction',
                aggfunc=agg_func,
            ).plot(
                title=f'Compute ratio ({n} vector size)',
                kind='line',
                xlabel='Number of processes',
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/compute_ratio_dim_{n}.svg')
            plt.close()

    def plot_repetitions(self, df: pd.DataFrame):
        for num_procs in [2 ** i for i in range(1, 6)]:
            vector_size = 4096

            data = df[(df['M'] == vector_size) & (df['numprocs'] == num_procs)]
            if data.shape[0] == 0:
                continue

            data.pivot_table(
                index='iteration',
                columns='implementation',
                values='runtime',
                aggfunc=agg_func,
            ).plot(
                title=f'Runtime across repetition number ({vector_size} vector size, {num_procs} nodes)',
                ylabel='Runtime (s)',
                xlabel='Iteration',
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
    df = df[df['implementation'] != 'rabenseifner-scatter']
    df_complete = df.copy()
    df = df[df['iteration'] > 1]

    pm.plot_runtime(df)
    # pm.plot_for_analysis(df)
    pm.plot_compute_ratio(df)
    pm.plot_mem_usage(df)
    pm.plot_repetitions(df_complete)
