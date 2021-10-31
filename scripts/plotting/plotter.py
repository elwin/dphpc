import json
import os
from pprint import pprint
import matplotlib.pyplot as plot
from numpy import log


ALLGATHER_KEY = "all-gather"
ALLREDUCE_KEY = "all-reduce"

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


class PlotMananager:
    def __init__(self, filePath: str):
        self.filePath = filePath
        self.benchmarkList = []
        self.agBenchmarkList = []
        self.arBenchmarkList = []
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
                    "runtime": log(p)*n**2,
                    "runtimes": [],
                    "timestamp": ""
                }

                input_ag = {
                    "N": n,
                    "M": n,
                    "errors": [],
                    "name": "all-gather",
                    "numprocs": p,
                    "runtime": (p-1)*(n+n),
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
        print("finished loading points")
        self.sortBenchMarks("N", "numprocs")
        print("finished sorting points")
        # self.splitBenchmarkList()

    def loadBenchmarks(self):
        # read file
        with open(self.filePath, 'r') as myfile:
            for line in myfile:
                obj = json.loads(line)
                benchmark = Benchmark(obj)
                self.benchmarkList.append(benchmark)
                if benchmark.runtime > self.maxRuntime:
                    self.maxRuntime = benchmark.runtime

        self.sortBenchMarks("N", "M")
        # self.splitBenchmarkList()

    def splitBenchmarkList(self):
        for benchmark in self.benchmarkList:
            if benchmark.name == ALLGATHER_KEY:
                self.agBenchmarkList.append(benchmark)
            else:
                self.arBenchmarkList.append(benchmark)

    def sortBenchMarks(self, firstOrder: str, secondOrder: str = None):
        if secondOrder:
            self.benchmarkList.sort(key= lambda u: u.json[secondOrder])
        self.benchmarkList.sort(key= lambda u: u.json[firstOrder])

    def generate2DPlot(self, listType: str = None):
        print("starting 2D plot")
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
        plot.savefig(listType + '_benchmark_plot.png')
        print("finished 2D plot")

    def mapRuntimeToColor(self, runtime:int):
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
            y.append(ag.N)
            x.append(ag.numprocs)
            color.append("green")

        for i in range(len(self.arBenchmarkList)):
            ar = self.arBenchmarkList[i]
            if y[i] == ar.N and x[i] == ar.numprocs:
                if self.agBenchmarkList[i].runtime > ar.runtime:
                    color[i] = "red"
        ax.scatter(x, y, c=color, cmap='viridis')

        plot.savefig('comparison_benchmark_plot.png')

pm = PlotMananager("./input_files_samples/first_file.txt")
pm.loadBenchmarks()
pm.generate2DPlot()
pm.generateComparisonPlot()