# Distributed Sum of Outer Product (DSOP)

## Meetings

Mondays, 16:15 until finished We often reserved CHN H 43.2 or alternatively
on [Zoom](https://ethz.zoom.us/j/69785702508).

## Code

All code related to solving DSOP is located in the folder [`/code`](code).

### Pre-Requisites

You'll need the following, perhaps also a bit more:

- [cmake](https://cmake.org/install/) with a somewhat recent version (I have 3.21.0)
- C++, standard 14 or higher would be nice (gcc, clang, ... should work)
- Something to execute `Makefile`'s (probably already installed)

### Run targets

This assumes you're located in `/code` (`cd code`)

- To build & run all tests, run `make test`
- To build & run the main file, `make main`

Targets are defined in the [`CMakeLists.txt`](code/CMakeLists.txt) file, take a look there for additional stuff.

I use Jetbrain's CLion - it nicely recognises all targets from the `CMakeLists.txt` file and allows to run & debug tests
individually. I can recommend it, but other approaches should of course also work (perhaps even better, let me know if
so).

### Etiquette rules

- To the `master` branch, only push code that builds with succeeding tests, perhaps make your own branch such
  as `elwin/master` to keep non-running code
- Nevertheless, merge to `master` as often as possible to avoid surprising your team-members and to avoid merge conflict

## Readings

- [Getting started with clusters (ETH)](https://scicomp.ethz.ch/wiki/Getting_started_with_clusters)
- [Scientific Benchmarking of Parallel Computing Systems](http://spcl.inf.ethz.ch/Teaching/2021-dphpc/hoefler-scientific-benchmarking.pdf)