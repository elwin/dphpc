# Code

## Pre-Requisites

You'll need the following, perhaps also a bit more:

- [cmake](https://cmake.org/install/) >= 3.19
- C++17 compiler ~~(gcc, clang, ... should work)~~ (gcc 9)
- Something to execute `Makefile`'s (probably already installed)
- [Open MPI](https://www.open-mpi.org/) >= 4

## Run targets

This assumes you're located in `/code` (`cd code`)

- To build & run all tests, run `make test`
- To build & run the main file, `make main`

Targets are defined in the [`CMakeLists.txt`](CMakeLists.txt) file, take a look there for additional stuff.

I use Jetbrain's CLion - it nicely recognises all targets from the `CMakeLists.txt` file and allows to run & debug tests
individually. I can recommend it, but other approaches should of course also work (perhaps even better, let me know if
so).

## Use GCC

In case you get some unexpected error, and you're not using gcc 9 or higher, perhaps it might be worth a try to switch.
For that, after installing the compiler, you must set the following env variables (perhaps you must change the path to
the compiler):

```shell
CXX=g++-9
CC=gcc-9
```

e.g. by adding those to your `~/.zshrc` / `~/.bashrc` using `export CXX=...` or simply setting them in your current
shell, e.g. `CXX=/usr/local/bin/g++-9  CC=gcc-9 make test`.
