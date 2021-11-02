# Distributed Sum of Outer Product (DSOP)

## Project Description

The goal of this project is to study the problem of Distributed Sum of Outer Product. In this problem we have P hosts
p1, p2, ..., pp, and each pi has two vectors Ai and Bi. We are interested in computing the sum of all outer products of
Ai and Bi.

At the end of the computation, each of the P hosts needs to have a copy of the matrix G. The main goal of this project
is to design and implement (with MPI) different efficient algorithms to compute G. The students should provide a cost
model for their algorithms, and benchmark them on real systems. The algorithms could take into account heterogeneity in
the interconnection network (i.e., different links might have a different bandwidth), and/or the sparsity of the input
vectors Ai and Bi.

## Meetings

Mondays, 16:15 until finished. We often reserved CHN H 43.2 or alternatively
on [Zoom](https://ethz.zoom.us/j/69785702508).

For meetings with our supervisors, use this [Link](https://ethz.zoom.us/my/sashkboos).

## Code

All code related to solving DSOP, including a `readme.md`, is located in the folder [`/code`](code).

## Run on cluster

There's a bunch of scripts in the [`/euler`](euler) folder, and a `readme.md` explaining how to use them.

## Results

Some (ongoing) benchmark results can be found in the [`/results`](https://github.com/elwin/dphpc_results/) folder.

## Etiquette rules

- To the `master` branch, only push code that builds with succeeding tests, perhaps make your own branch such
  as `elwin/master` to keep non-running code
- Nevertheless, merge to `master` as often as possible to avoid surprising your team-members and to avoid merge conflict

## Readings

- [Getting started with clusters (ETH)](https://scicomp.ethz.ch/wiki/Getting_started_with_clusters)
- [Scientific Benchmarking of Parallel Computing Systems](http://spcl.inf.ethz.ch/Teaching/2021-dphpc/hoefler-scientific-benchmarking.pdf)
- [MPI Tutorials](https://mpitutorial.com/)
