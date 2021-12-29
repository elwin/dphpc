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
from scipy.stats import bootstrap
import ast

from itertools import product

from matplotlib.axes import Axes

agg_func = np.median

IMPL_NAMES = {
        'g-rabenseifner-allgather': 'g-rabenseifner-dsop'
}

def get_impl_label(impl: str):
    return IMPL_NAMES.get(impl, impl)

def impl_labels(impls: List[str]):
    return list(map(get_impl_label, impls))


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

        agg_func = get_agg_func(func_key)

        selected_impls = ['allgather', 'allreduce', 'allreduce-ring', 'g-rabenseifner-allgather']
        all_impls = ['allgather', 'allreduce', 'allreduce-ring', 'g-rabenseifner-allgather', "g-rabenseifner-subgroup-2", "g-rabenseifner-subgroup-4", "g-rabenseifner-subgroup-8"]

        df = df[df['implementation'].isin(all_impls)]

        sizes = sorted(df['N'].unique().tolist())
        processes = sorted(df['numprocs'].unique().tolist())
        impls = df['implementation'].unique().tolist()

        print(f"sizes: {sizes}")
        print(f"processes: {processes}")
        print(f"impls: {impls}")

        self.prefix = 'report/analysis'

        # for N, num_procs in product(sizes, processes):
        #     self.plot_report_all_node_configs(df, N, num_procs, impls, 95)

        # All measurements within the same repetition are aggregated as a single measurement
        df = df.groupby(['N', 'numprocs', 'repetition', 'implementation']).agg(
            runtime=pd.NamedAgg(column="runtime", aggfunc=agg_func)
        )
        df = df.reset_index()

        # self.plot_report_boxviolin('box', True, df, processes, impls, 95)
        # self.plot_report_boxviolin('violin', False, df, processes, impls, 95)
        #
        # fig, (ax_left, ax_right) = plt.subplots(ncols=2, sharey=True, figsize=(24, 9))
        # self.plot_report_boxviolin_comparison(True, df, ax_left, 8000, 48, impls, 95)
        # self.plot_report_boxviolin_comparison(False, df, ax_right, 8000, 48, impls, 95)
        # ax_left.set_ylabel('Runtime (s)', rotation=90, fontsize=12)
        # fig.suptitle("Runtime plots for N = M = 8000 and num_procs = 48")
        # self.plot_and_save('cmp', None, None)
        #
        # self.plot_report_runtime(df, processes, impls, 75)
        # self.plot_report_violin_cmp_all(df, 'N', sizes, 'numprocs', selected_impls)
        #
        self.prefix = 'report'

        self.plot_report_violin_cmp_all(df, 'numprocs', [16, 32, 48], 'N', selected_impls)
        self.plot_report_violin_cmp_all(df, 'numprocs', [48], 'N', selected_impls)
        self.plot_report_speedup(df, 'numprocs', [16, 32, 48], 'N', selected_impls, 'allreduce', False)
        self.plot_report_speedup(df, 'numprocs', [16, 32, 48], 'N', selected_impls, 'allreduce', True)

        for p in [50, 75, 90, 95]:
            data = df[df['implementation'].isin(selected_impls)]
            self.plot_runtime_with_errorbars_subplots(data, filter_key='implementation', index_key='N', line_key='numprocs', func_key='percentile', percentile=p)
            self.plot_runtime_with_errorbars_subplots(data, filter_key='implementation', index_key='numprocs', line_key='N', func_key='percentile', percentile=p)

        for p in [50, 90]:
            self.plot_report_runtime_errorbars(df, 'numprocs', [16, 32, 48], 'N', selected_impls, p, 0.95)
            self.plot_report_runtime_errorbars(df, 'numprocs', [48], 'N', selected_impls, p, 0.95)

        self.prefix = 'report/subgroup'

        for p in [50, 75, 90, 95]:
            data = df[df['implementation'].isin(['g-rabenseifner-allgather', "g-rabenseifner-subgroup-2", "g-rabenseifner-subgroup-4", "g-rabenseifner-subgroup-8"])]
            self.plot_runtime_with_errorbars_subplots(data, filter_key='implementation', index_key='N', line_key='numprocs', func_key='percentile', percentile=p)
            self.plot_runtime_with_errorbars_subplots(data, filter_key='implementation', index_key='numprocs', line_key='N', func_key='percentile', percentile=p)

    def plot_report_runtime_errorbars(self, df: pd.DataFrame, filter_key: str, filter_values: List[int], index_key: str, impls: List[str], percentile: float, CI_bound: float):
        ncols = len(filter_values)
        fig, axs = plt.subplots(ncols=ncols, sharey=True, figsize=(8.3 * ncols, 7), squeeze=False)
        axs = axs.flat
        data = self.CI_bootstrap(df, filter_key, index_key, 'implementation', percentile, CI_bound)
        color_dict = self.map_colors(impls)

        for i, filter_value in enumerate(filter_values):
            ax = axs[i]
            plt_data = data[data[filter_key] == filter_value]

            for algo in impls:
                self.plot_runtime_with_errorbars_single(plt_data, ax, algo, index_key, color_dict)

            ax.set_title(f'{filter_key} = {filter_value}')
            ax.set_xlabel(None)
            if i == 0:
                ax.set_ylabel('Runtime [s]')
                ax.legend()
            else:
                ax.set_ylabel(None)
                ax.legend().remove()

        
        if index_key == 'N':
            fig.supxlabel('Number of vector elements N = M')
            fig.suptitle(f"{int(CI_bound * 100)}% confidence interval over {percentile}th percentile")

        name = f"runtime_{filter_key}_{'_'.join(map(str, filter_values))}_{index_key}_percentile_{percentile}_CI_{CI_bound}_with_errorbar"
        self.plot_and_save_with_log(name, None, None)

    def calculate_speedup(self, df: pd.DataFrame, impls: List[str], baseline: str):
        """
        Calculates the speedup for all datapoints against the given baseline.

        For this it aggregates over all iterations in a repetition with the median to get a single observation.

        For each observation, the speedup is calculated.

        The resulting DataFrame does not contain the baseline but contains the speedup for each observation.
        """
        df = df[df['implementation'].isin(impls + [baseline])]
        # Take median inside each repetition
        data = df.groupby(['N', 'numprocs', 'implementation', 'repetition']).agg(
            {'runtime': np.median})
        data.reset_index(inplace=True)

        baseline_df = data[data['implementation'] == baseline]
        data = data[data['implementation'] != baseline]

        def compute_speedup(baseline_runtimes, vec_size, num_procs, runtime):
            baseline_runtime = baseline_runtimes[(baseline_runtimes['N'] == vec_size) & (baseline_runtimes['numprocs'] == num_procs)]
            if baseline_runtime.empty:
                return math.nan
            else:
                return list(baseline_runtime['runtime'])[0] / runtime

        # compute the speedups
        repetitions = sorted(data['repetition'].unique().tolist())

        for rep in repetitions:
            baseline_runtimes = baseline_df.query('`repetition` == @rep')
            rep_df = data[data['repetition'] == rep]
            data.loc[data['repetition'] == rep, 'speedup'] = rep_df.apply(lambda x: compute_speedup(baseline_runtimes, x['N'], x['numprocs'], x["runtime"]), axis=1)

        return data

    def plot_report_speedup(self, df: pd.DataFrame, filter_key: str, filter_values: List[int], index_key: str, impls: List[str], baseline: str, violin: bool):
        impls = [impl for impl in impls if impls != baseline]

        data = self.calculate_speedup(df, impls, baseline)
        color_dict = self.map_colors(impls)

        if not violin:
            # Take the median over all speedups
            data = data.groupby([filter_key, index_key, 'implementation']).agg(
                speedup=pd.NamedAgg(column="speedup", aggfunc=agg_func)
            )
            data.reset_index(inplace=True)

        fig, axs = plt.subplots(ncols=len(filter_values), sharey=True, figsize=(25, 7), squeeze=False)
        axs = axs.flat

        for i, filter_value in enumerate(filter_values):
            ax = axs[i]
            plt_data = data[data[filter_key] == filter_value]

            if not violin:
                self.plot_speedup_single(plt_data, ax, baseline, index_key, color_dict)
            else:
                self.plot_speedup_single_violin(plt_data, ax, baseline, index_key)

            ax.set_title(f'{filter_key} = {filter_value}')
            ax.set_xlabel(None)
            if i == 0:
                ax.set_ylabel('Speedup')
            else:
                ax.set_ylabel(None)
                ax.legend().remove()

        
        fig.suptitle(f"Speedup against '{get_impl_label(baseline)}'")
        fig.supxlabel('Number of vector elements N = M')
        violin_infix = '_violin' if violin else ''
        name = f"speedup_plot{violin_infix}_{index_key}_{filter_key}__baseline_{baseline}"
        self.plot_and_save(name, None, None)

    def plot_report_violin_cmp_all(self, df: pd.DataFrame, filter_key: str, filter_values: List[int], index_key: str, impls: List[str]):
        ncols = len(filter_values)
        fig, axs = plt.subplots(ncols=ncols, sharey=True, figsize=(8.3 * ncols, 7), squeeze=False)
        axs = axs.flat
        for i, filter_value in enumerate(filter_values):
            ax = axs[i]
            ax.set_title(f'{filter_key} = {filter_value}')
            self.plot_report_violin_cmp_single(df[df[filter_key] == filter_value], ax, index_key, impls)
            ax.set_xlabel(None)
            if i == 0:
                ax.set_ylabel('Runtime [s]')
            else:
                ax.set_ylabel(None)
                ax.legend().remove()

        fig.supxlabel('Number of vector elements N = M')
        self.plot_and_save_with_log(f'cmp_{filter_key}_{"_".join(map(str, filter_values))}', None, None)

    def plot_report_violin_cmp_single(self, df: pd.DataFrame, ax: Axes, index_key: str, impls: List[str],
            percentile: float = 95, y_key: str='runtime'):
        data = df[df['implementation'].isin(impls)]
        impl_names = impl_labels(impls)

        if y_key == 'runtime':
            outlier_cutoff = 40
            outliers = data[data['runtime'] >= outlier_cutoff]
            if not outliers.empty:
                print(f"Outliers ({len(outliers.index)}):")
                print(outliers.to_string())
            data = data[data['runtime'] < outlier_cutoff]
        sns.violinplot(x=index_key, y=y_key, hue='implementation', data=data, ax=ax, inner='box', scale_hue=True, cut=0, scale='width', palette=self.map_colors(impls), saturation=0.7, linewidth=0.75)

        handles, labels = ax.get_legend_handles_labels()
        sns.boxplot(x=index_key, y=y_key, hue='implementation', data=data, ax=ax, showfliers=False, showbox=False, whis=[100 - percentile, percentile], palette=self.map_colors(impls))
        
        # Replace legend with state before boxplot to avoid duplicates
        # We also replace the algo names with the proper names from IMPL_NAMES
        ax.legend(handles, labels[:-len(impl_names)] + impl_names, loc="upper left")

    def plot_report_all_node_configs(self, df: pd.DataFrame, N: int, num_procs: int, impls: List[str],
                                     percentile: float = 95):
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

            self.plot_report_boxviolin_comparison(False, filtered, ax, N, num_procs, impls, percentile)
            ax.set_title(f'Repetition {rep}')
            ax.set_xlabel('')

        self.plot_and_save(f'all_configs_{N}_{num_procs}', None, None)

    def plot_straggler_violin(self, df: pd.DataFrame, N: int, num_procs: int, impl: str, key: str = 'runtimes',
                              percentile: float = 95):
        data = df.query("`N` == @N & `numprocs` == @num_procs & `implementation` == @impl")
        # data = df[(df['N'] == N) & (df['numprocs'] == num_procs) & (df['implementation'] == impl)]
        repetitions = sorted(data['repetition'].unique().tolist())
        num_reps = len(repetitions)

        perc_high = percentile / 100
        perc_low = 1 - perc_high
        violin_quantiles = [perc_low, perc_high]
        box_whiskers = (violin_quantiles[0] * 100, violin_quantiles[1] * 100)

        ncols = 7
        nrows = math.ceil(num_reps / ncols)

        standard_figsize = (45, 30)
        smaller_figsize = (18, 10)
        fig, axes = plt.subplots(nrows=nrows, ncols=ncols, sharex=True, sharey=True, figsize=smaller_figsize)
        axs = axes.flat

        fig.suptitle(f"{impl}, N={N}, numprocs={num_procs}, {key}", fontsize=15)

        for i, rep in enumerate(repetitions):
            ax = axs[i]
            filtered = data.query("`repetition` == @rep")

            if filtered.empty:
                continue

            # if boxplot:
            #     filtered.boxplot(column=key, by=['iteration'], ax=ax, whis=box_whiskers, notch=False,
            #                  showfliers=False)
            # else:

            iterations = filtered['iteration'].unique()
            xticks = range(len(iterations))
            # l = []
            # for it in iterations:
            #     el = filtered[filtered['iteration'] == it].iloc[0][key]
            #     transformed_el = list(map(int, ast.literal_eval( el )))
            #     l.append(transformed_el)
            l = [list(map(int, ast.literal_eval(filtered[filtered['iteration'] == it].iloc[0][key]))) for it in
                 iterations]
            # l = [filtered[filtered['iteration'] == it][key] for it in iterations]
            quantiles = [violin_quantiles] * len(xticks)

            if min([len(x) for x in l]) == 0:
                return

            data_l = [[f"{it}", it_idx, el] for it_idx, it in enumerate(iterations) for el_idx, el in enumerate(l[it_idx])]
            print_df = pd.DataFrame(data_l, columns=["iteration", "idx", "runtime"])

            sns.violinplot(data=print_df, x="runtime", y="iteration", ax=ax, quantiles=quantiles, showmedians=True, showextrema=False, orient="h")

            ax.set_title(f'Repetition {rep}')
            ax.set_ylabel('iteration')
            ax.set_xlabel(key)

        self.plot_and_save(f'stragglers_{impl}_{N}_{num_procs}_{key}', None, None, subplot_adjust=True)


    def plot_report_runtime(self, df: pd.DataFrame, num_processes: List[int], impls: List[str], percentile: int = 95):
        data_errors = df.groupby(['numprocs', 'implementation', 'N']).agg(
            runtime=pd.NamedAgg(column="runtime", aggfunc=np.median),
            yerr_low=pd.NamedAgg(column="runtime",
                                 aggfunc=lambda p: np.median(p) - np.percentile(p, [100 - percentile])),
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

    def plot_report_boxviolin_comparison(self, boxplot: bool, df: pd.DataFrame, ax: Axes, N: int, num_procs: int,
                                         impls: List[str], percentile=95):
        perc_high = percentile / 100
        perc_low = 1 - perc_high
        violin_quantiles = [perc_low, perc_high]
        box_whiskers = (violin_quantiles[0] * 100, violin_quantiles[1] * 100)

        data = df[(df['N'] == N) & (df['numprocs'] == num_procs)]
        if boxplot:
            data.boxplot(column='runtime', by=['implementation'], ax=ax, whis=box_whiskers, notch=False,
                         showfliers=False)
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

    def plot_report_boxviolin(self, name: str, boxplot: bool, df: pd.DataFrame, num_processes: List[int],
                              impls: List[str], percentile=95):
        perc_high = percentile / 100
        perc_low = 1 - perc_high
        violin_quantiles = [perc_low, perc_high]
        box_whiskers = (violin_quantiles[0] * 100, violin_quantiles[1] * 100)

        num_rows = len(num_processes)
        num_cols = len(impls)

        fig, axes = plt.subplots(nrows=num_rows, ncols=num_cols, sharex=True, sharey='row', figsize=(36, 13),
                                 squeeze=False)

        for i, numprocs in enumerate(num_processes):
            for j, impl in enumerate(impls):
                data = df[(df['implementation'] == impl) & (df['numprocs'] == numprocs)]
                if boxplot:
                    data.boxplot(column='runtime', by=['N'], ax=axes[i, j], whis=box_whiskers, notch=False,
                                 showfliers=False)
                else:
                    xticks = data['N'].unique().tolist()
                    l = [data[data['N'] == x]['runtime'].to_list() for x in xticks]
                    quantiles = [violin_quantiles] * len(xticks)
                    axes[i, j].violinplot(l, positions=xticks, quantiles=quantiles, showmedians=True, showextrema=False,
                                          widths=[xticks[-1] / len(xticks) * 0.5] * len(xticks))

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
        color_dict = self.map_colors(df['implementation'].unique())

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
                color_map=color_dict,
            )
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/runtime_{i}_{filter_key}_{func_key}.svg')
            plt.close()

    def plot_speedup(self, df: pd.DataFrame,
                     filter_key: str = 'N',
                     index_key: str = 'numprocs',
                     baseline: str = 'allreduce'):
        # aggregate data
        data = df.groupby([filter_key, index_key, 'implementation', 'repetition']).agg(
            {'runtime': np.median})
        data.reset_index(inplace=True)
        data = data.groupby([filter_key, index_key, 'implementation']).agg(
            runtime=pd.NamedAgg(column="runtime", aggfunc=agg_func)
        )
        data.reset_index(inplace=True)

        color_dict = self.map_colors(data['implementation'].unique())

        baseline_df = data[data['implementation'] == baseline]

        def compute_speedup(filter_key_value, index_key_value, runtime):
            baseline_runtime = baseline_df[
                (baseline_df[filter_key] == filter_key_value) & (baseline_df[index_key] == index_key_value)]
            speedup = list(baseline_runtime['runtime'])[0] / runtime
            return speedup

        # compute the speedups
        data['speedup'] = 1.0
        data['speedup'] = data.apply(lambda x: compute_speedup(x[filter_key], x[index_key], x["runtime"]), axis=1)

        for i in data[filter_key].unique():
            plt_data = data[data[filter_key] == i]

            self.plot_speedup_single(plt_data, None, baseline, index_key, color_dict)

            plt.tight_layout(pad=3.)
            plt.legend()  # loc="upper left"
            # plt.legend(bbox_to_anchor=(1,1)) # loc="upper left"
            plt.xlabel(index_key)
            plt.ylabel('Speedup')
            plt.title(f'Speedup against {baseline} ({filter_key} = {i})')

            name = f"speedup_plot_{index_key}_{filter_key}_{i}_baseline_{baseline}"
            self.plot_and_save(name, None, None)

        # TODO
        # baseline_runtimes = data[data['implementation'] == baseline]

    def plot_speedup_single(self, df: pd.DataFrame, ax: Axes, baseline: str, index_key: str, color_dict):
        df = df[df['implementation'] != baseline]
        plt_data_pivot = df.pivot(
            index=index_key,
            columns='implementation',
            values='speedup'
        )
        plt_data_pivot.plot(color=[color_dict.get(x) for x in df['implementation']], ax=ax, marker='.', markersize=16)

        impl_names = impl_labels(df['implementation'].unique().tolist() + [baseline])

        base = ax if ax else plt
        base.axhline(1, linestyle='--', color='grey', linewidth=1, label=get_impl_label(baseline))
        base.legend(labels=impl_names)

    def plot_speedup_single_violin(self, df: pd.DataFrame, ax: Axes, baseline: str, index_key: str):
        df = df[df['implementation'] != baseline]
        impls = df['implementation'].unique().tolist()

        ax.axhline(1, linestyle='--', color='grey', linewidth=1, label=get_impl_label(baseline))
        ax.legend()

        self.plot_report_violin_cmp_single(df, ax, index_key, impls, 95, 'speedup')

    def plot_runtime_with_errorbars_subplots(self, df: pd.DataFrame,
                                             filter_key='implementation',
                                             index_key='numprocs',
                                             line_key='N',
                                             func_key='median',
                                             percentile=99.,
                                             CI_bound=0.95):
        if func_key == 'percentile':
            func_key = f"percentile_{percentile}"

        # define suplots
        subplot_vals = df[filter_key].unique()
        max_subplots_per_row = 4
        nrows = int(math.ceil(len(subplot_vals) / max_subplots_per_row))
        ncols = int(min(len(subplot_vals), max_subplots_per_row))
        fig, axes = plt.subplots(nrows=nrows,
                                 ncols=ncols, sharex=True, sharey=True, figsize=(25, 7))

        data = self.CI_bootstrap(df, filter_key, index_key, line_key, percentile, CI_bound)
        color_dict = self.map_colors(data['implementation'].unique(), n_shades=len(data[line_key].unique()))

        # make multiple subplots
        for i, i_val in enumerate(data[filter_key].unique()):
            plt_data = data[data[filter_key] == i_val]

            subplot_row = int(i / max_subplots_per_row)
            subplot_col = i % max_subplots_per_row
            if nrows > 1:
                axref = axes[subplot_row, subplot_col]
            else:
                axref = axes[i]

            if filter_key == 'implementation':
                axref.set_title(get_impl_label(i_val))
            else:
                axref.set_title(i_val)

            if i == 0:
                axref.set_ylabel('Runtime [s]')
            else:
                axref.set_ylabel(None)

            # make multiple lines in suplot
            for j, j_val in enumerate(plt_data[line_key].unique()):
                # loop over algos to set color right if key not already an algo
                subplt_data = plt_data[plt_data[line_key] == j_val]
                for algo in subplt_data['implementation'].unique():
                    # get color from the multiple shades
                    c = color_dict.get(algo)[j]

                    data_filtered = subplt_data[subplt_data['implementation'] == algo]

                    axref.errorbar(x=data_filtered[index_key], y=data_filtered['agg_runtime'],
                                   yerr=data_filtered[
                                       ['yerr_low', 'yerr_high']].to_numpy().transpose(),
                                   color=c, label=f"{line_key} = {j_val}", fmt=':', alpha=0.9, capsize=3,
                                   capthick=1)
                    axref.fill_between(x=data_filtered[index_key], y1=data_filtered['CI_low'],
                                       y2=data_filtered['CI_high'],
                                       color=c, alpha=0.25)
            axref.legend(loc="upper left")
            axref.set_xticks(sorted(data_filtered[index_key].unique().tolist()))

        if func_key.startswith('percentile'):
            fig.suptitle(f"{int(CI_bound * 100)}% confidence interval over {percentile}th percentile")
        if index_key == 'N':
            fig.supxlabel('Number of vector elements N = M')
        elif index_key == 'numprocs':
            fig.supxlabel('Number of processes')
        else:
            fig.supxlabel(index_key)

        # plt.ylim((data['CI_low'].min(), data['CI_high'].max()))
        # plt.yscale('log', nonposy='clip')
        plt.tight_layout()
        # plt.legend(bbox_to_anchor=(1,1)) # loc="upper left"

        output_dir = self.output_dir if self.prefix is None else f'{self.output_dir}/{self.prefix}'
        pathlib.Path(output_dir).mkdir(parents=True, exist_ok=True)

        for format in ["svg", "png"]:
            plt.savefig(
                f'{output_dir}/runtime_subplots_{index_key}_{filter_key}_{line_key}_{func_key}_CI_{CI_bound}_with_errorbar.{format}')
        plt.yscale('log')
        plt.tight_layout()
        for format in ["svg", "png"]:
            plt.savefig(
                f'{output_dir}/runtime_subplots_{index_key}_{filter_key}_{line_key}_{func_key}_CI_{CI_bound}_with_errorbar_log_scale.{format}')
        plt.close()

        return

    def CI_bootstrap(self, df: pd.DataFrame, filter_key: str, index_key: str, line_key: str, percentile: float, CI_bound: float):
        agg_func = get_agg_func('percentile', percentile)
        def CI(data):
            data = (data,)
            # data.to_numpy().reshape((-1, data.shape[0]))
            # take agg_func as statistic
            res = bootstrap(data, agg_func, confidence_level=CI_bound, n_resamples=1000, method='percentile',
                            axis=0).confidence_interval
            return res.low, res.high

        data = df
        # aggregate the data in one repetition using the median
        data_aggregated = data.groupby([filter_key, index_key, line_key, 'repetition']).agg(
            {'runtime': np.median})
        data = data_aggregated.reset_index()
        data_CI = data.groupby([filter_key, index_key, line_key]).agg(
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
        return data

    def plot_runtime_with_errorbars_single(self,
                                    df: pd.DataFrame,
                                    ax: Axes,
                                    algo: str,
                                    filter_key: str,
                                    color_dict):
        data_filtered = df[df['implementation'] == algo]

        ax.errorbar(x=data_filtered[filter_key], y=data_filtered['agg_runtime'],
                    yerr=data_filtered[['yerr_low', 'yerr_high']].to_numpy().transpose(),
                    color=color_dict.get(algo), label=get_impl_label(algo), fmt=':', alpha=0.9, capsize=3, capthick=1)
        ax.fill_between(x=data_filtered[filter_key], y1=data_filtered['CI_low'], y2=data_filtered['CI_high'],
                                color=color_dict.get(algo), alpha=0.25)
    def plot_runtime_with_errorbars(self,
                                    df: pd.DataFrame,
                                    filter_key='N',
                                    index_key='numprocs',
                                    func_key='median',
                                    percentile=99.,
                                    CI_bound=0.95,
                                    powers_of_two=False):
        if func_key == 'percentile':
            func_key = f"percentile_{percentile}"

        data = self.CI_bootstrap(df, filter_key, index_key, 'implementation', percentile, CI_bound)
        color_dict = self.map_colors(data['implementation'].unique())

        for i in data[index_key].unique():
            if powers_of_two and not math.log(i, 2).is_integer():
                continue
            plt_data = data[data[index_key] == i]

            fig, ax = plt.subplots()
            # ax = plt.gca()
            for algo in plt_data['implementation'].unique():
                self.plot_runtime_with_errorbars_single(plt_data, ax, algo, filter_key, color_dict)

            # plt.ylim((data['CI_low'].min(), data['CI_high'].max()))
            # plt.yscale('log', nonposy='clip')
            plt.tight_layout(pad=3.)
            plt.legend()  # loc="upper left"
            # plt.legend(bbox_to_anchor=(1,1)) # loc="upper left"
            plt.xlabel(filter_key)
            plt.ylabel('Runtime (s)')
            plt.title(f'Runtime ({index_key} = {i})')

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

                fig, axes = plt.subplots(nrows=n_rows, ncols=n_cols, sharex=True, sharey=True, figsize=(24, 13),
                                         squeeze=False)

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
            plt.legend(loc="upper left", bbox_to_anchor=(1, 1))
            plt.tight_layout()
            plt.savefig(f'{self.output_dir}/runtime_{i}_{filter_key}_{func_key}_with_scatter.svg')
            plt.close()

    @staticmethod
    def map_colors(implementation_types, n_shades=1):
        # hard-code base algorithms
        base_algorithms = ["allgather", "allreduce", "allreduce-ring", "g-rabenseifner-allgather",
                           "g-rabenseifner-subgroup-2", "g-rabenseifner-subgroup-4", "g-rabenseifner-subgroup-8"]
        for i in implementation_types:
            if i not in base_algorithms:
                base_algorithms.append(i)
        # good choices: Set2, tab10, husl
        colors = sns.color_palette("tab10", n_colors=len(base_algorithms))
        brightness = list(np.linspace(0.5, 1.5, num=n_shades))
        # colors = ['red', 'blue', 'green', 'purple', 'yellow', 'pink', 'gray', 'orange']

        # Always assign the same color to the same algorithm
        base_dict = {impl:colors[i] for (i, impl) in enumerate(base_algorithms)}

        color_dict = {}
        for impl in implementation_types:
            color = base_dict[impl]
            if n_shades == 1:
                color_dict[impl] = color
            else:
                colors_list = []
                for j in range(n_shades):
                    bright = brightness[j]
                    r = min(1., color[0] * bright)
                    g = min(1., color[1] * bright)
                    b = min(1., color[2] * bright)
                    colors_list.append((r, g, b))
                color_dict[impl] = colors_list
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

    def plot_and_save_with_log(self, name: str, width: float = 10, height: float = 5, subplot_adjust: bool = False):
        self.plot_and_save(name, width, height, subplot_adjust, False)
        plt.yscale('log')
        self.plot_and_save(name + "_log_scale", width, height, subplot_adjust, False)

    def plot_and_save(self, name: str, width: float = 10, height: float = 5, subplot_adjust: bool = False, close: bool = True):
        if width is not None and height is not None:
            plt.gcf().set_size_inches(width, height)
        if subplot_adjust:
            plt.tight_layout(pad=2.)
            fig = plt.gcf()
            fig.subplots_adjust(top=0.9)
        else:
            plt.tight_layout()
        # plt.legend(bbox_to_anchor=(1, 1))
        output_dir = self.output_dir if self.prefix is None else f'{self.output_dir}/{self.prefix}'
        pathlib.Path(output_dir).mkdir(parents=True, exist_ok=True)
        plt.savefig(f'{output_dir}/{name}.png')
        plt.savefig(f'{output_dir}/{name}.svg')
        if close:
            plt.close()

        print(f"\033[32mFinished plotting '{name}'\033[0m")

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
                marker='o',
            )
            self.plot_and_save(f'runtime_{i}')

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
                marker='o',
            )
            self.plot_and_save(f'runtime_dim_{n}')

    def plot_mem_usage(self, df: pd.DataFrame):
        print("Plotting memory usage")
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
                kind='line',
                ylabel='Memory Usage (GB)',
                xlabel='Input Dimension',
                marker='o',
            )
            self.plot_and_save(f'mem_usage_{i}')

    def plot_compute_ratio(self, df: pd.DataFrame):
        print("Plotting compute ratio")
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
                ylim=(0, 1),
                marker='o',
            )
            self.plot_and_save(f'compute_ratio_{i}')

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
                ylim=(0, 1),
                marker='o',
            )

            self.plot_and_save(f'compute_ratio_dim_{n}')

    def plot_iterations(self, df: pd.DataFrame):
        print("Plotting iterations")
        for num_procs in [2 ** i for i in range(1, 6)]:
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

        if prefix is None:
            self.prefix = "stragglers"
        else:
            self.prefix = f"{prefix}/stragglers"
        sizes = [8000]
        processes = [48, 32]
        impls = df['implementation'].unique().tolist()
        straggler_df = df[df['repetition'] < 8]
        for N, num_procs, impl in product(sizes, processes, impls):
            for key in ["runtimes", "runtimes_compute", "runtimes_mpi"]:
                self.plot_straggler_violin(straggler_df, N, num_procs, impl, key, 95)
        self.prefix = prefix

        self.plot_runtime_with_errorbars_subplots(df, filter_key='implementation', index_key='numprocs', line_key='N',
                                                  func_key='percentile', percentile=50.)
        self.plot_runtime_with_errorbars_subplots(df, filter_key='implementation', index_key='N', line_key='numprocs',
                                                  func_key='percentile', percentile=50.)

        self.plot_speedup(df, filter_key='N', index_key='numprocs', baseline='allreduce')
        self.plot_speedup(df, filter_key='numprocs', index_key='N', baseline='allreduce')

        CI_bounds = [0.99, 0.95, 0.9]
        percentiles = [99., 50., 5., 10., 75., 90., 95.]
        if prefix is None:
            self.prefix = "errorbars"
        else:
            self.prefix = f"{prefix}/errorbars"
        for percentile in percentiles:
            for CI_bound in CI_bounds:
                self.plot_runtime_with_errorbars(df, CI_bound=CI_bound, func_key='percentile', percentile=percentile)
        self.prefix = prefix
        self.plot_runtime_with_errorbars(df, CI_bound=0.95, func_key='percentile', percentile=50.)
        self.plot_runtime_with_errorbars(df, filter_key='numprocs', index_key='N', CI_bound=0.95, func_key='percentile',
                                         percentile=50.)

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
            df = pd.json_normalize(json.loads(df.to_json(orient="records")))  # flatten nested json
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


    # size_df = df.groupby(["N", "implementation", "numprocs", "repetition"]).size()
    # size_df = size_df.reset_index()
    # n_runs = size_df.groupby(["N", "implementation", "numprocs"]).size()
    # print(n_runs)

    pm.plot_for_report(df)


    pm.plot_all(df, prefix="all")


    selected_impls = ['allgather', 'allreduce', 'allreduce-ring', 'g-rabenseifner-allgather']
    df_filtered = df[df['implementation'].isin(selected_impls)]
    pm.plot_all(df_filtered, prefix="filtered")





    # pm.plot_for_analysis(df)

    # cutoff = 12000
    # outliers = df[df['job.runtime'] >= cutoff]
    # df = df[df['job.runtime'] < cutoff]

    # print("Plotting non-native implementation")
    # pm.plot_all(df[~df['implementation'].str.contains('native')])
    # print("Plotting allreduce implementations")
    # pm.plot_all(df[df['implementation'].str.startswith('allreduce')], prefix='native')
    # print("Plotting allgather implementations")
    # pm.plot_all(df[df['implementation'].str.startswith('allgather')], prefix='native_allgather')
    # pm.plot_all(outliers, prefix='outliers')

    # print("Plotting job stats")
    # pm.plot_job_stats(df)
