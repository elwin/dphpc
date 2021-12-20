import dataclasses
import json
import pathlib
from typing import List
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import pandas.io.json
import math
import seaborn as sns
import os

from itertools import product

from matplotlib.axes import Axes

agg_func = np.median

def get_agg_func(func_key: str, percentile=99.):
    if func_key == 'mean':
        return np.mean
    elif func_key == 'median':
        return np.median
    elif func_key == 'max':
        return np.max
    elif func_key == 'min':
        return np.min
    elif func_key == 'percentile':
        return PlotManager.percentile_agg_func(percentile)
    else:
        return np.mean


@dataclasses.dataclass
class PlotManager:
    output_dir: str
    prefix: str = None

    def plot_for_report(self, df: pd.DataFrame, func_key='median'):
        print("Plotting for report")
        self.prefix = 'report'

        selected_impls = ['allgather', 'allreduce', 'g-rabenseifner-allgather']
        selected_impls = ['allgather', 'allreduce', 'allreduce-ring', 'g-rabenseifner-allgather']

        df = df[df['implementation'].isin(selected_impls)]

        sizes = df['N'].unique().tolist()
        processes = df['numprocs'].unique().tolist()
        impls = df['implementation'].unique().tolist()

        print(f"sizes: {sizes}")
        print(f"processes: {processes}")
        print(f"impls: {impls}")

        for N, num_procs in product(sizes, processes):
            self.plot_report_all_node_configs(df, N, num_procs, impls, 95)

        # All measurements within the same repetition are aggregated as a single measurement
        df = df.groupby(['N', 'numprocs', 'repetition', 'implementation']).agg(
                runtime=pd.NamedAgg(column="runtime", aggfunc=agg_func)
                )
        df = df.reset_index()

        self.plot_report_boxviolin('box', True, df, processes, impls, 95)
        self.plot_report_boxviolin('violin', False, df, processes, impls, 95)

        fig, (ax_left, ax_right) = plt.subplots(ncols=2, sharey=True, figsize=(24, 9))
        self.plot_report_boxviolin_comparison(True, df, ax_left, 8000, 48, impls, 95)
        self.plot_report_boxviolin_comparison(False, df, ax_right, 8000, 48, impls, 95)
        ax_left.set_ylabel('Runtime (s)', rotation=90, fontsize=12)
        fig.suptitle("Runtime plots for N = M = 8000 and num_procs = 48")
        self.plot_and_save('cmp', None, None)

        self.plot_report_runtime(df, processes, impls, 75)

    def plot_report_all_node_configs(self, df: pd.DataFrame, N: int, num_procs: int, impls: List[str], percentile: float=95):
        data = df.query("`N` == @N & `numprocs` == @num_procs")
        repetitions = sorted(data['repetition'].unique().tolist())
        num_reps = len(repetitions)

        ncols = 6
        nrows = math.ceil(num_reps / ncols)

        fig, axes = plt.subplots(nrows=nrows, ncols=ncols, sharex=True, figsize=(45, 16))
        axs = axes.flat

        for i, rep in enumerate(repetitions):
            ax = axs[i]
            filtered = data.query("`repetition` == @rep")

            if filtered.empty:
                continue
            print(f"{N=}, {num_procs=}, {i=}, {rep=}")

            self.plot_report_boxviolin_comparison(False, filtered, ax, N, num_procs, impls, percentile)
            ax.set_title(f'Repetition {rep}')
            ax.set_xlabel('')

        self.plot_and_save(f'all_configs_{N}_{num_procs}', None, None)

    def plot_report_speedup(self, df: pd.DataFrame, baseline: str, num_processes: List[int], impls: List[str], percentile: float=75):
        assert baseline not in impls
        data = df.groupby(['numprocs', 'implementation', 'N']).agg(
            runtime=pd.NamedAgg(column="runtime", aggfunc=np.median),
        )
        data.reset_index(inplace=True)

        # TODO
        # baseline_runtimes = data[data['implementation'] == baseline]

    def plot_report_runtime(self, df: pd.DataFrame, num_processes: List[int], impls: List[str], percentile: int=95):
        data_errors = df.groupby(['numprocs', 'implementation', 'N']).agg(
            runtime=pd.NamedAgg(column="runtime", aggfunc=np.median),
            yerr_low=pd.NamedAgg(column="runtime", aggfunc=lambda p: np.median(p) - np.percentile(p, [100 - percentile])),
            yerr_high=pd.NamedAgg(column="runtime", aggfunc=lambda p: np.percentile(p, [percentile]) - np.median(p)),
        )
        data_errors.reset_index(inplace=True)

        color_dict = self.map_colors(impls)

        for i in num_processes:
            fig, ax = plt.subplots()
            for impl in impls:
                data = data_errors[(data_errors['numprocs'] == i) & (data_errors['implementation'] == impl)]
                if data.shape[0] == 0:
                    continue
                ax.errorbar(x=data['N'], y=data['runtime'], yerr=data[['yerr_low', 'yerr_high']].to_numpy().transpose(),
                            color=color_dict.get(impl),
                            label=impl,
                            fmt=':',
                            alpha=0.9,
                            capsize=3,
                            capthick=1)

            plt.xlabel('Input Dimension')
            plt.ylabel('Runtime (s)')
            plt.title(f'Runtime ({i} nodes)')
            self.plot_and_save(f'runtime_{i}')

    def plot_report_boxviolin_comparison(self, boxplot: bool, df: pd.DataFrame, ax: Axes, N: int, num_procs: int, impls: List[str], percentile=95):
        perc_high = percentile / 100
        perc_low = 1 - perc_high
        violin_quantiles = [perc_low, perc_high]
        box_whiskers = (violin_quantiles[0] * 100, violin_quantiles[1] * 100)

        data = df[(df['N'] == N) & (df['numprocs'] == num_procs)]
        if boxplot:
            data.boxplot(column='runtime', by=['implementation'], ax=ax, whis=box_whiskers, notch=False, showfliers=False)
        else:
            xticks = range(len(impls))
            l = [data[data['implementation'] == x]['runtime'].to_list() for x in impls]
            quantiles = [violin_quantiles] * len(xticks)

            if min([len(x) for x in l]) == 0:
                return

            ax.violinplot(l, positions=xticks, quantiles=quantiles, showmedians=True, showextrema=False)
            ax.set_xticks(xticks)
            ax.set_xticklabels(impls)

        ax.set_xlabel('[impl]')
        ax.set_title('')

    def plot_report_boxviolin(self, name: str, boxplot: bool, df: pd.DataFrame, num_processes: List[int], impls: List[str], percentile=95):
        perc_high = percentile / 100
        perc_low = 1 - perc_high
        violin_quantiles = [perc_low, perc_high]
        box_whiskers = (violin_quantiles[0] * 100, violin_quantiles[1] * 100)

        num_rows = len(num_processes)
        num_cols = len(impls)

        fig, axes = plt.subplots(nrows=num_rows, ncols=num_cols, sharex=True, sharey='row', figsize=(36, 13), squeeze=False)

        for i, numprocs in enumerate(num_processes):
            for j, impl in enumerate(impls):
                data = df[(df['implementation'] == impl) & (df['numprocs'] == numprocs)]
                if boxplot:
                    data.boxplot(column='runtime', by=['N'], ax=axes[i, j], whis=box_whiskers, notch=False, showfliers=False)
                else:
                    xticks = data['N'].unique().tolist()
                    l = [data[data['N'] == x]['runtime'].to_list() for x in xticks]
                    quantiles = [violin_quantiles] * len(xticks)
                    axes[i, j].violinplot(l, positions=xticks, quantiles=quantiles, showmedians=True, showextrema=False, widths=[xticks[-1] / len(xticks) * 0.5] * len(xticks))

        for i, val_i in enumerate(num_processes):
            axes[i, 0].set_ylabel(val_i, rotation=90, fontsize=12)

        for j, val_j in enumerate(impls):
            axes[0, j].set_title(val_j)

            for i in range(0, len(num_processes)):
                if i != 0:
                    axes[i, j].set_title('')
                axes[i, j].set_xlabel('')

        fig.suptitle('Runtimes')
        self.plot_and_save(name, None, None)

    def plot_for_analysis(self, df: pd.DataFrame, func_key='median'):
        print("Plotting for analysis")
        self.plot_runtime_with_scatter(df, 'numprocs', 'N', func_key=func_key)
        self.plot_runtime_with_scatter(df, 'N', 'numprocs', func_key=func_key)
        self.plot_runtime_by_key(df, 'N', 'numprocs', func_key=func_key)
        self.plot_runtime_by_key(df, 'numprocs', 'N', func_key=func_key)

    def plot_runtime_by_key(self,
                            df: pd.DataFrame,
                            filter_key='N',
                            index_key='numprocs',
                            func_key='mean',
                            percentile=99.,
                            powers_of_two=True):
        agg_func = get_agg_func(func_key, percentile)

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

    def plot_runtime_with_errorbars(self,
                                    df: pd.DataFrame,
                                    filter_key='N',
                                    index_key='numprocs',
                                    func_key='median',
                                    percentile=99.,
                                    CI_bound=0.95,
                                    powers_of_two=False):
        agg_func = get_agg_func(func_key, percentile)

        if func_key == 'percentile':
            func_key = f"percentile_{percentile}"

        def CI(data):
            data = (data,)
            # data.to_numpy().reshape((-1, data.shape[0]))
            # take agg_func as statistic
            res = bootstrap(data, agg_func, confidence_level=CI_bound, n_resamples=1000, method='percentile', axis=0).confidence_interval
            return res.low, res.high

        data = df
        # aggregate the data
        # data_aggregated = data.groupby([filter_key, index_key, 'implementation', 'repetition']).agg(
        #     {'runtime': agg_func})
        # data = data_aggregated.reset_index()
        data_CI = data.groupby([filter_key, index_key, 'implementation']).agg(
            confidence_interval=pd.NamedAgg(column="runtime", aggfunc=CI),
            agg_runtime=pd.NamedAgg(column="runtime", aggfunc=agg_func),
            # median_runtime=pd.NamedAgg(column="runtime", aggfunc=np.median),
            # mean_runtime=pd.NamedAgg(column="runtime", aggfunc=np.mean)
        )
        data = data_CI.reset_index()
        data['CI_low'] = data.apply(lambda x: x.confidence_interval[0][0], axis=1)
        data['CI_high'] = data.apply(lambda x: x.confidence_interval[1][0], axis=1)
        data['yerr_low'] = data.apply(lambda x: np.abs(x.CI_low - x.agg_runtime), axis=1)
        data['yerr_high'] = data.apply(lambda x: np.abs(x.CI_high - x.agg_runtime), axis=1)

        color_dict = self.map_colors(data['implementation'].unique())

        for i in data[index_key].unique():
            if powers_of_two and not math.log(i, 2).is_integer():
                continue
            plt_data = data[data[index_key] == i]

            fig, ax = plt.subplots()
            # ax = plt.gca()
            for algo in plt_data['implementation'].unique():
                data_filtered = plt_data[plt_data['implementation'] == algo]
                # print(data_filtered[['CI_low', 'CI_high']].to_numpy().transpose())
                # # ax.plot(data_filtered[filter_key], data_filtered['runtime'], marker='+', color=color_dict.get(algo))
                # ax.errorbar(x=data_filtered[filter_key], y=data_filtered['runtime'],
                #             yerr=data_filtered[['CI_low', 'CI_high']].to_numpy().transpose(),
                #             color=color_dict.get(algo), label=algo, fmt='none', capsize=2.5)

                ax.errorbar(x=data_filtered[filter_key], y=data_filtered['agg_runtime'],
                            yerr=data_filtered[['yerr_low', 'yerr_high']].to_numpy().transpose(),
                            color=color_dict.get(algo), label=algo, fmt=':', alpha=0.9, capsize=3, capthick=1)
                ax.fill_between(x=data_filtered[filter_key], y1=data_filtered['CI_low'], y2=data_filtered['CI_high'],
                                color=color_dict.get(algo), alpha=0.25)

            # plt.ylim((data['CI_low'].min(), data['CI_high'].max()))
            # plt.yscale('log', nonposy='clip')
            plt.tight_layout(pad=3.)
            plt.legend(bbox_to_anchor=(1,1)) # loc="upper left"
            plt.xlabel('Input Dimension')
            plt.ylabel('Runtime (s)')
            plt.title(f'Runtime ({i} nodes)')

            output_dir = self.output_dir if self.prefix is None else f'{self.output_dir}/{self.prefix}'
            pathlib.Path(output_dir).mkdir(parents=True, exist_ok=True)

            plt.savefig(f'{output_dir}/runtime_{index_key}_{i}_{filter_key}_{func_key}_CI_{CI_bound}_with_errorbar.svg')
            plt.yscale('log')
            plt.savefig(
                f'{output_dir}/runtime_{index_key}_{i}_{filter_key}_{func_key}_CI_{CI_bound}_with_errorbar_log_scale.svg')
            plt.close()

    def plot_subplot_histograms(self, df: pd.DataFrame, log: bool = True, n_bins: int = 25):
        # TODO: Put histogram sizes also into the arguments
        agg_func = np.mean

        data = df.groupby(['N', 'numprocs', 'repetition', 'implementation']).agg(
            runtime=pd.NamedAgg(column="runtime", aggfunc=agg_func)
        )
        data = data.reset_index()

        # make a plot per node configuration and one per algorithm
        keys = [('numprocs', 'implementation'), ('implementation', 'numprocs')]
        for key in keys:
            for key_val in data[key[0]].unique():
                plt_data = data[data[key[0]] == key_val]
                rows = plt_data[key[1]].unique().tolist()
                cols = plt_data['N'].unique().tolist()
                rows.sort()
                cols.sort()
                n_rows = len(rows)
                n_cols = len(cols)
                value_range = (np.min(df['runtime']), np.max(df['runtime']))
                if log:
                    value_range = (np.min(np.log(df['runtime'])), np.max(np.log(df['runtime'])))
                    plt_data = plt_data.apply(lambda x: np.log(x) if x.name == 'runtime' else x)

                fig, axes = plt.subplots(nrows=n_rows, ncols=n_cols, sharex=True, sharey=True, figsize=(24, 13), squeeze=False)



                for i, val_i in enumerate(rows):
                    for j, val_j in enumerate(cols):
                        filtered_data = plt_data[
                            (plt_data[key[0]] == key_val) & (plt_data[key[1]] == val_i) & (
                                    plt_data['N'] == val_j)]
                        axes[i, j].hist(x=filtered_data['runtime'], bins=n_bins, color="skyblue", range=value_range)

                # set plot labels
                for i, val_i in enumerate(rows):
                    axes[i, 0].set_ylabel(val_i, rotation=90, fontsize=9)
                for j, val_j in enumerate(cols):
                    axes[0, j].set_title(val_j)

                output_dir = self.output_dir if self.prefix is None else f'{self.output_dir}/{self.prefix}'
                pathlib.Path(output_dir).mkdir(parents=True, exist_ok=True)

                log_str = ''
                log_title = ''
                if log:
                    log_str = '_log_scale'
                    log_title = ' (log of runtime)'

                fig.suptitle(f"Runtime Histogram for {key[0]}={key_val} {log_title}")

                plt.tight_layout()
                plt.savefig(f'{output_dir}/histogram_runtime_{key_val}_{key[0]}_{n_bins}_bins{log_str}.svg')
                plt.close()

    def plot_runtime_with_scatter(self,
                                  df: pd.DataFrame,
                                  filter_key='N',
                                  index_key='numprocs',
                                  func_key='median',
                                  percentile=99.,
                                  powers_of_two=True):
        agg_func = get_agg_func(func_key, percentile)

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
            plt.gcf().set_size_inches(10, 5)
            plt.legend(loc="upper left", bbox_to_anchor=(1,1))
            plt.tight_layout()
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
    def percentile_agg_func(percentile):
        perc = [percentile]

        def percentile(p, axis=0):
            return np.percentile(p, perc, axis=axis)

        return percentile

    def plot_and_save(self, name: str, width: float = 10, height: float = 5):
        if width is not None and height is not None:
            plt.gcf().set_size_inches(width, height)
        plt.legend(bbox_to_anchor=(1,1))
        plt.tight_layout()
        output_dir = self.output_dir if self.prefix is None else f'{self.output_dir}/{self.prefix}'
        pathlib.Path(output_dir).mkdir(parents=True, exist_ok=True)
        plt.savefig(f'{output_dir}/{name}.svg')
        plt.savefig(f'{output_dir}/{name}.png')
        plt.close()

    def plot_runtime(self, df: pd.DataFrame):
        for i in [2**i for i in range(1, 6)]:
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
                marker='o',
            )
            self.plot_and_save(f'runtime_{i}')

        for n in [2**n for n in range(4, 14)]:
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
                marker='o',
            )
            self.plot_and_save(f'runtime_dim_{n}')

    def plot_mem_usage(self, df: pd.DataFrame):
        print("Plotting memory usage")
        for i in [2**i for i in range(1, 6)]:
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
                kind='line',
                ylabel='Memory Usage (GB)',
                xlabel='Input Dimension',
                marker='o',
            )
            self.plot_and_save(f'mem_usage_{i}')

    def plot_compute_ratio(self, df: pd.DataFrame):
        print("Plotting compute ratio")
        for i in [2**i for i in range(1, 6)]:
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
                ylim=(0, 1),
                marker='o',
            )
            self.plot_and_save(f'compute_ratio_{i}')

        for n in [2**n for n in range(4, 14)]:
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
                ylim=(0, 1),
                marker='o',
            )

            self.plot_and_save(f'compute_ratio_dim_{n}')

    def plot_iterations(self, df: pd.DataFrame):
        print("Plotting iterations")
        for num_procs in [2**i for i in range(1, 6)]:
            vector_size = 8000

            data = df[(df['M'] == vector_size) & (df['numprocs'] == num_procs)]
            if data.shape[0] == 0:
                continue

            data.pivot_table(
                index='iteration',
                columns='implementation',
                values='runtime',
                aggfunc=agg_func,
            ).plot(
                title=f'Runtime across iteration number ({vector_size} vector size, {num_procs} nodes)',
                ylabel='Runtime (s)',
                logy=True,
                xlabel='Iteration',
            )
            self.plot_and_save(f'runtime_iteration_{num_procs}')

    def plot_all(self, df: pd.DataFrame, prefix=None):
        self.prefix = prefix

        CI_bounds = [0.99, 0.95, 0.9]
        percentiles = [99., 50., 5., 10., 75., 90., 95.]
        self.prefix = "errorbars"
        for percentile in percentiles:
            for CI_bound in CI_bounds:
                self.plot_runtime_with_errorbars(df, CI_bound=CI_bound, func_key='percentile', percentile=percentile)
        self.prefix = prefix
        self.plot_runtime_with_errorbars(df, CI_bound=0.95, func_key='percentile', percentile=50.)
        # for CI_bound in CI_bounds:
        #     self.plot_runtime_with_errorbars(df, CI_bound=CI_bound, func_key='median')
        #     # self.plot_runtime_with_errorbars(df, CI_bound=CI_bound, func_key='mean')
        #     # self.plot_runtime_with_errorbars(df, CI_bound=CI_bound, func_key='max')
        #     # self.plot_runtime_with_errorbars(df, CI_bound=CI_bound, func_key='min')

        # for n_bins in [50]:
        #     self.plot_subplot_histograms(df, n_bins=n_bins, log=False)
        #     self.plot_subplot_histograms(df, n_bins=n_bins, log=True)
    
        self.plot_runtime(df)
        self.plot_compute_ratio(df)
        # self.plot_mem_usage(df)
        # self.plot_iterations(df)

    def plot_job_stats(self, df: pd.DataFrame):
        self.prefix = 'job'

        data = df[['N', 'numprocs', 'job.turnaround_time']]

        data['queue_time'] = data.apply(lambda x: x['job.turnaround_time'] / (60 * 60), axis=1)

        # df['job.runtime']
        for x_data in [('N', 'numprocs'), ('numprocs', 'N')]:
            df.plot.scatter(
                x=x_data[0],
                y='job.runtime',
                c=x_data[1],
                ylabel='runtime (s)'
            )
            self.plot_and_save(f"runtime_hist_{x_data[0]}")

            data.plot.scatter(
                x=x_data[0],
                y='queue_time',
                c=x_data[1],
                ylabel='queueing time (h)'
            )
            self.plot_and_save(f"queueing_hist_{x_data[0]}")


def plot(input_files: List[str], input_dir: str, output_dir: str):
    sns.set()

    pm = PlotManager(output_dir=output_dir)

    aggregated_file = f"{input_dir}/aggregated.csv"
    if os.path.isfile(aggregated_file):
        df = pd.read_csv(aggregated_file)
    else:
        dfs = []
        for input_file in input_files:
            df = pandas.io.json.read_json(input_file, lines=True)
            df = pd.json_normalize(json.loads(df.to_json(orient="records"))) # flatten nested json
            dfs.append(df)
        df = pd.concat(dfs)
        df.to_csv(aggregated_file)

    df = df.rename(columns={'name': 'implementation'})
    df['job.mem_max'] /= 1000
    df['job.mem_max_avg'] = df['job.mem_max'] / df['numprocs']
    df['runtime_compute'] /= 1_000_000
    df['runtime'] /= 1_000_000
    df['fraction'] = df['runtime_compute'] / df['runtime']
    # Drop the first iteration because it is a warmup iteration
    df = df[df['iteration'] > 0]

    pm.plot_for_report(df)

    # pm.plot_for_analysis(df)

    # cutoff = 12000
    # outliers = df[df['job.runtime'] >= cutoff]
    # df = df[df['job.runtime'] < cutoff]

    # print("Plotting non-native implementation")
    # pm.plot_all(df[~df['implementation'].str.contains('native')])
    # print("Plotting allreduce implementations")
    # pm.plot_all(df[df['implementation'].str.startswith('allreduce')], prefix='native')
    # pm.plot_all(outliers, prefix='outliers')

    # print("Plotting job stats")
    # pm.plot_job_stats(df)
