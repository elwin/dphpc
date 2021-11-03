import argparse
import json
import os
import pathlib
from pprint import pprint
import matplotlib.pyplot as plot
from numpy import log, log2, number
import numpy as np
import copy

ALLGATHER_KEY = "allgather"
ALLREDUCE_KEY = "allreduce"
AR_BUTTERFLY_KEY = "allreduce-butterfly"
NAME_COLOR_MAP = {
    ALLREDUCE_KEY: 'red',
    ALLGATHER_KEY: 'green',
    AR_BUTTERFLY_KEY: 'blue'
}


class Benchmark:
    def __init__(self, json):
        self.M = json["M"]
        self.N = json["N"]
        # self.errors = json["errors"]
        self.name = json["name"]
        self.numprocs = json["numprocs"]
        self.runtime = json["runtime"]
        self.runtimes = json["runtimes"]
        self.timestamp = json["timestamp"]
        self.json = json

    def print(self):
        print("{")
        print("N: " + str(self.N))
        print("numprocs: " + str(self.numprocs))
        print("runtime: " + str(self.runtime))
        print("name: " + str(self.name))
        print("}")


class PlotMananager:
    def __init__(self, inputDir: str, outputDir: str):
        self.inputDir = inputDir
        self.outputDir = outputDir
        self.benchmarkList = []
        self.agBenchmarkList = []
        self.arBenchmarkList = []
        self.otherBenchmarkLists = {}
        self.maxRuntime = 0

    def loadSampleBenchmarks(self):
        for n_index in range(1, 10000):
            for p_index in range(1, 100):
                n = n_index * 10
                p = p_index * 10
                input_ar = {
                    "N": n,
                    "M": n,
                    "errors": [],
                    "name": "all-reduce",
                    "numprocs": p,
                    "runtime": log(p) * n ** 2,
                    "runtimes": [],
                    "timestamp": ""
                }

                input_ag = {
                    "N": n,
                    "M": n,
                    "errors": [],
                    "name": "all-gather",
                    "numprocs": p,
                    "runtime": (p - 1) * (n + n),
                    "runtimes": [],
                    "timestamp": ""
                }

                ag = Benchmark(input_ag)
                ar = Benchmark(input_ar)
                self.benchmarkList.append(ar)
                self.benchmarkList.append(ag)
                if ag.runtime > self.maxRuntime:
                    self.maxRuntime = ag.runtime
                if ar.runtime > self.maxRuntime:
                    self.maxRuntime = ar.runtime
                self.agBenchmarkList.append(ag)
                self.arBenchmarkList.append(ar)
        self.sortBenchMarks("N", "numprocs")
        # self.splitBenchmarkList()

    def loadBenchmarks(self, useAverage=True, fileName=""):
        # read file
        if useAverage:
            files = os.listdir(self.inputDir)
        else:
            files = [fileName]
        initialBenchMark = []
        for file in files:
            with open(f"{self.inputDir}/{file}", 'r') as myfile:
                for line in myfile:
                    obj = json.loads(line)
                    benchmark = Benchmark(obj)
                    initialBenchMark.append(benchmark)
                    if benchmark.runtime > self.maxRuntime:
                        self.maxRuntime = benchmark.runtime
        if useAverage:
            self.benchmarkList = self.generateAverageBenchmarkList(initialBenchMark)
        else:
            self.benchmarkList = initialBenchMark
        self.sortBenchMarks("N", "M")
        self.splitBenchmarkList()

    def generateAverageBenchmarkList(self, benchmarks):
        allReadyCalculated = []
        benchmarks_averaged = []
        for elem in benchmarks:
            param_key = str(elem.N) + " " + str(elem.numprocs) + " " + elem.name
            if not param_key in allReadyCalculated:
                averaged_elem = copy.deepcopy(elem)
                average_runtime = 0
                numberOfRuns = 0
                for i in range(len(benchmarks)):
                    elem_inner = benchmarks[i]
                    param_key_inner = str(elem_inner.N) + " " + str(elem_inner.numprocs) + " " + elem_inner.name
                    if param_key_inner == param_key:
                        numberOfRuns += 1
                        average_runtime += elem_inner.runtime
                averaged_elem.runtime = average_runtime / numberOfRuns
                benchmarks_averaged.append(averaged_elem)
                allReadyCalculated.append(param_key)

        return benchmarks_averaged

    def splitBenchmarkList(self):
        for benchmark in self.benchmarkList:
            if benchmark.name == ALLGATHER_KEY:
                self.agBenchmarkList.append(benchmark)
            elif benchmark.name == ALLREDUCE_KEY:
                self.arBenchmarkList.append(benchmark)
            else:
                if benchmark.name in self.otherBenchmarkLists.keys():
                    self.otherBenchmarkLists[benchmark.name].append(benchmark)
                else:
                    self.otherBenchmarkLists[benchmark.name] = [benchmark]


    def sortBenchMarks(self, firstOrder: str, secondOrder: str = None):
        if secondOrder:
            self.benchmarkList.sort(key=lambda u: u.json[secondOrder])
        self.benchmarkList.sort(key=lambda u: u.json[firstOrder])

    def generate2DPlot(self, listType: str = None):
        if listType == ALLGATHER_KEY:
            benchmarkPlotList = self.agBenchmarkList
        elif listType == ALLREDUCE_KEY:
            benchmarkPlotList = self.arBenchmarkList
        else:
            listType = "all"

        benchmarkPlotList = self.benchmarkList
        x = []
        y = []
        color = []
        fix, ax = plot.subplots()
        for benchmark in benchmarkPlotList:
            y.append(benchmark.N)
            x.append(benchmark.numprocs)
            color.append(self.mapRuntimeToColor(benchmark.runtime))

        # ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Number of Processes")
        ax.set_ylabel("Size of Vectors")
        ax.scatter(x, y, c=color, cmap='viridis')
        plot.savefig(f"{self.outputDir}/{listType}_benchmark_plot.png")

    def mapRuntimeToColor(self, runtime: int):
        return runtime * 255.0 / self.maxRuntime

    def generateComparisonPlot(self):
        fix, ax = plot.subplots()
        # ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("Number of Processes")
        ax.set_ylabel("Size of Vectors")
        x = []
        y = []
        color = []
        for ag in self.agBenchmarkList:
            for ar in self.arBenchmarkList:
                if ag.N == ar.N and ag.numprocs == ar.numprocs:
                    y.append(ag.N)
                    x.append(ag.numprocs)
                    if ag.runtime > ar.runtime:
                        color.append("red")
                    else:
                        color.append("green")

        ax.scatter(x, y, c=color, cmap='viridis')

        plot.savefig(f'{self.outputDir}/comparison_benchmark_plot.png')

    def plotPerNumProcesses(self, plot_type="numprocs", sort_key="N", x_scale="linear", y_scale="linear"):
        self.sortListByKey(self.agBenchmarkList, sort_key)
        self.sortListByKey(self.arBenchmarkList, sort_key)
        ag_split = self.splitBenchMarksByKey(self.agBenchmarkList, plot_type)
        ar_split = self.splitBenchMarksByKey(self.arBenchmarkList, plot_type)
        others_split = self.genOtherBenchmarkSplitList(self.otherBenchmarkLists, plot_type)

        for key in ag_split.keys():
            if key in ar_split.keys():
                fix, ax = plot.subplots()
                ax.set_yscale(y_scale)
                ax.set_xscale(x_scale)
                self.printSingleLine(ar_split, sort_key, key, ax)
                self.printSingleLine(ag_split, sort_key, key, ax)
                for others_key in others_split.keys():
                    if key in others_split[others_key].keys():
                        self.printSingleLine(others_split[others_key], sort_key, key, ax)


                if plot_type == "numprocs":
                    ax.set_xlabel("Vector Size")
                else:
                    ax.set_xlabel("Number of Processes")
                ax.set_ylabel("Runtime")
                plot.title(plot_type + " = " + str(key))
                plot.legend(loc="upper left")
                plot.savefig(f"{self.outputDir}/{key}_{plot_type}_benchmark_plot.png")
                plot.close()

    def printSingleLine(self, split_list, sort_key, key, ax):
        (x, y) = self.extractParamAndRuntime(split_list[key], sort_key)
        name = split_list[key][0].name
        color = NAME_COLOR_MAP[name]
        if len(x) == 1:
            ax.scatter(x, y, color=color, label=name)
        elif len(x) > 1:
            ax.plot(x, y, color=color, label=name)

    def genOtherBenchmarkSplitList(self, list, plot_type):
        splitOtherBenchMarks = {}
        for key in list.keys():
            splitOtherBenchMarks[key] = self.splitBenchMarksByKey(list[key], plot_type)
        return splitOtherBenchMarks


    def extractParamAndRuntime(self, list, sort_key):
        x = []
        y = []
        for elem in list:
            if sort_key == "N":
                x.append(elem.N)
            else:
                x.append(elem.numprocs)
            y.append(elem.runtime)
        return (x, y)

    def splitBenchMarksByKey(self, list, key):
        benchmarksSplit = {}
        for elem in list:
            if key == "numprocs":
                dict_key = elem.numprocs
            else:
                dict_key = elem.N

            if dict_key in benchmarksSplit.keys():
                benchmarksSplit[dict_key].append(elem)
            else:
                benchmarksSplit[dict_key] = [elem]
        return benchmarksSplit

    def sortListByKey(self, list, key):
        list.sort(key=lambda u: u.json[key])

    def plotTheoreticalComparison(self, fixed_key, fixed_value, upper_limit, x_scale='linear', y_scale='log'):
        fig, ax = plot.subplots()
        ax.set_yscale(y_scale)
        ax.set_xscale(x_scale)
        x = np.linspace(2, upper_limit)
        plot.style.use('default')

        if fixed_key == "N":
            ar_fixed = fixed_value ** 2
            ag_fixed = fixed_value * 2
            y_ar = np.multiply(ar_fixed, np.log2(x))
            y_ag = np.multiply(ag_fixed, np.subtract(x, 1))
        else:
            y_ar = np.multiply(log2(fixed_value), np.power(x))
            y_ag = np.multiply(fixed_value - 1, np.multiply(x, 2))

        plot.style.use('ggplot')

        ax.plot(x, y_ar, color='red', label='All-Reduce')
        ax.plot(x, y_ag, color='green', label='All-Gather')

        plot.title("Theoretical prediction for " + fixed_key + " = " + str(fixed_value))
        plot.legend(loc="upper left")
        plot.savefig(f"{self.outputDir}/{fixed_value}_{fixed_key}_theoretical_plot.png")
        plot.close()


def main():
    parser = argparse.ArgumentParser(description='Run some benchmarks')
    parser.add_argument('-d', '--dir', type=str, default="results/tmp",
                        help="Directory containing results (both in and output")
    args = parser.parse_args()

    results_dir = args.dir
    output_dir = f"{results_dir}/plots"
    input_dir = f"{results_dir}/parsed"
    pathlib.Path(output_dir).mkdir(parents=True, exist_ok=True)
    pm = PlotMananager(inputDir=input_dir, outputDir=output_dir)
    pm.loadBenchmarks()
    pm.generate2DPlot()
    pm.generateComparisonPlot()
    pm.plotPerNumProcesses(plot_type='numprocs', x_scale='linear', y_scale='log')
    pm.plotPerNumProcesses(plot_type='N', sort_key="numprocs", x_scale='linear', y_scale='log')
    # pm.plotTheoreticalComparison("N", 4092, 256)


if __name__ == '__main__':
    main()
