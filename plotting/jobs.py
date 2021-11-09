import json

import matplotlib.pyplot as plt
import pandas as pd
import pandas.io.json

results_dir = 'results/06_gooder_measures'
input_dir = f'{results_dir}/parsed'
output_dir = f'{results_dir}/plots'
repetitions = 3


def plot_runtime(df: pd.DataFrame):
    for i in [2 ** i for i in range(1, 6)]:
        data = df[df['numprocs'] == i]

        data.pivot_table(
            index='N',
            columns='implementation',
            values='runtime',
        ).plot(
            title=f'Runtime ({i} nodes)',
            kind='line',
            logy=True,
            ylabel='Runtime (s)',
            xlabel='Input Dimension',
        )
        plt.tight_layout()
        plt.savefig(f'{output_dir}/runtime_{i}.svg')
        plt.close()

    for n in [2 ** n for n in range(4, 14)]:
        data = df[df['N'] == n]

        data.pivot_table(
            index='numprocs',
            columns='implementation',
            values='runtime',
        ).plot(
            title=f'Runtime ({n} vector size)',
            kind='line',
            logy=True,
            ylabel='Runtime (s)',
            xlabel='Number of processes',
        )
        plt.tight_layout()
        plt.savefig(f'{output_dir}/runtime_dim_{n}.svg')
        plt.close()


def plot_mem_usage(df: pd.DataFrame):
    for i in [2 ** i for i in range(1, 6)]:
        data = df[df['numprocs'] == i]
        data = data[['N', 'implementation', 'job.mem_max', 'job.mem_requested']]

        data.pivot_table(
            index='N',
            columns='implementation',
            values='job.mem_max',
        ).plot(
            title=f'Memory Usage ({i} nodes)',
            kind='bar',
            logy=True,
            ylabel='Memory Usage (MB)',
            xlabel='Input Dimension',
        )
        plt.tight_layout()
        plt.savefig(f'{output_dir}/mem_usage_{i}.svg')
        plt.close()


def plot_compute_ratio(df: pd.DataFrame):
    for i in [2 ** i for i in range(1, 6)]:
        data = df[df['numprocs'] == i]

        data.pivot_table(
            index='N',
            columns='implementation',
            values='fraction',
        ).plot(
            title=f'Compute ratio ({i} nodes)',
            kind='bar',
        )
        plt.tight_layout()
        plt.savefig(f'{output_dir}/compute_ratio_{i}.svg')
        plt.close()


def main():
    dfs = []
    for i in range(repetitions):
        df = pandas.io.json.read_json(f'{input_dir}/{i}.json', lines=True)
        df = pd.json_normalize(json.loads(df.to_json(orient="records")))
        df = df.rename(columns={'name': 'implementation'})
        dfs.append(df)

    df = pd.concat(dfs)
    df['fraction'] = df['runtime_compute'] / df['runtime']

    plot_runtime(df)
    plot_compute_ratio(df)
    plot_mem_usage(df)


if __name__ == '__main__':
    main()
