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

## Code

All code related to solving DSOP is located in the folder [`/code`](code).

### Pre-Requisites

You'll need the following, perhaps also a bit more:

- [cmake](https://cmake.org/install/) >= 3.19
- C++17 compiler ~~(gcc, clang, ... should work)~~ (gcc 11)
- Something to execute `Makefile`'s (probably already installed)
- [Open MPI](https://www.open-mpi.org/) >= 4

### Run targets

This assumes you're located in `/code` (`cd code`)

- To build & run all tests, run `make test`
- To build & run the main file, `make main`

Targets are defined in the [`CMakeLists.txt`](code/CMakeLists.txt) file, take a look there for additional stuff.

I use Jetbrain's CLion - it nicely recognises all targets from the `CMakeLists.txt` file and allows to run & debug tests
individually. I can recommend it, but other approaches should of course also work (perhaps even better, let me know if
so).

### Use GCC

In case you get some unexpected error and you're not using gcc 11, perhaps it might be worth a try to switch. For that,
after installing the compiler, you must set the following env variables (perhaps you must change the path to the compiler):

```shell
CXX=g++-11
CC=gcc-11
```

e.g. by adding those to your `~/.zshrc` / `~/.bashrc` using `export CXX=...` or simply setting them in your current
shell, e.g. `CXX=/usr/local/bin/g++-11  CC=gcc-11 make test`.

## Running on the cluster

First, before accessing the cluster, one must be inside the ETH network. This can easily be achieved by using the ETH
VPN. As soon as we are inside the network, we can connect to the Euler cluster via SSH: `<nethz username>@euler.ethz.ch`

### Checking out the codebase

As git is available on the cluster, you can simply clone this repository into you personal home directory on the
cluster: `git clone https://github.com/elwin/dphpc`

> You can also send [@elwin](mailto:elwin.stephan@gmail.com) a copy of your public key on your Euler account and he'll add it as a deploy key to the repository. Afterwards, you can `git clone git@github.com:elwin/dphpc.git` without having to do any further authorization :crystal_ball:

### Preparations

Before we can build and run our code, we have to prepare the environment to use the correct versions of our build tools.
To do this, we will first switch to the new software stack by running:

```shell
env2lmod
```

Then we can load the required modules by using:

```shell
module load gcc/9.3.0 cmake/3.20.3 openmpi/4.0.2
```

Then you should be able to build the project with `make <target>`

### Running

```shell
# Directly run it on the local node
mpirun -np <number of processes> <binary>
# So for example:
mpirun -np 4 ./main -n 5 -m 5 -i allreduce

# You can also run it as a job on the cluster
bsub -n <number of processes> mpirun -np <number of processes> <binary>
# Example:
bsub -n 4 mpirun -np 4 ./main -n 5 -m 5 -i allreduce
```

### Etiquette rules

- To the `master` branch, only push code that builds with succeeding tests, perhaps make your own branch such
  as `elwin/master` to keep non-running code
- Nevertheless, merge to `master` as often as possible to avoid surprising your team-members and to avoid merge conflict


## Running Benchmark File

### Preparations

Create an ssh key for uploading to Euler.
```shell
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_euler
```
And upload the public key to the euler cluster.
```shell
ssh-copy-id -i $HOME/.ssh/id_ed25519_euler.pub    username@euler.ethz.ch
```
Make an ssh config file (or extend the current one if it already exists) under ```~/.ssh/config``` on local computer, and put the following content there:
```
Host euler
    HostName euler.ethz.ch
    User <eth-username>
    IdentityFile ~/.ssh/id_ed25519_euler
```
You can now ssh into the euler cluster using using ```ssh euler```

### Submit Jobs




## Readings

- [Getting started with clusters (ETH)](https://scicomp.ethz.ch/wiki/Getting_started_with_clusters)
- [Scientific Benchmarking of Parallel Computing Systems](http://spcl.inf.ethz.ch/Teaching/2021-dphpc/hoefler-scientific-benchmarking.pdf)
- [MPI Tutorials](https://mpitutorial.com/)
