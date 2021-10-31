import json
import os
from pprint import pprint
import matplotlib.pyplot as plot
from numpy import log


ALLGATHER_KEY = "allgather"
ALLREDUCE_KEY = "allreduce"

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
        self.splitBenchmarkList()

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
        print(len(self.agBenchmarkList))
        print(len(self.arBenchmarkList))
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

        plot.savefig('comparison_benchmark_plot.png')
    
    def plotPerNumProcesses(self, plot_type = "numprocs", sort_key = "N", x_scale = "linear", y_scale = "linear"):
        self.sortListByKey(self.agBenchmarkList, sort_key) 
        self.sortListByKey(self.arBenchmarkList, sort_key)
        ag_split = self.splitBenchMarksByKey(self.agBenchmarkList, plot_type)
        ar_split = self.splitBenchMarksByKey(self.arBenchmarkList, plot_type)

        for key in ag_split.keys():
            if key in ar_split.keys():
                fix, ax = plot.subplots()
                (x_ar, y_ar) = self.extractParamAndRuntime(ar_split[key], sort_key)
                (x_ag, y_ag) = self.extractParamAndRuntime(ag_split[key], sort_key)
                ax.set_yscale(y_scale)
                ax.set_xscale(x_scale)
                ax.plot(x_ar, y_ar, color='red')
                ax.plot(x_ag, y_ag, color='green')
                if plot_type == "numprocs":
                    ax.set_xlabel("Vector Size")
                else:
                    ax.set_xlabel("Number of Processes")
                ax.set_ylabel("Runtime")
                plot.savefig(str(key) + '_' + plot_type + '_benchmark_plot.png')
                plot.close()

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
        list.sort(key= lambda u: u.json[key])

pm = PlotMananager("./input_files_samples/1.json")
pm.loadBenchmarks()
pm.generate2DPlot()
pm.generateComparisonPlot()
pm.plotPerNumProcesses(plot_type='numprocs', x_scale = 'linear', y_scale = 'log')
pm.plotPerNumProcesses(plot_type='N', sort_key="numprocs", x_scale = 'linear', y_scale = 'log')