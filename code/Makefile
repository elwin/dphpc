# cannot be named "build", otherwise clashes with the target
# and never re-builds once created
OUT="build_output"

build:
	cmake -S . -B ${OUT}
	cmake --build ${OUT}

test: build
	cd ${OUT} && ctest

main: build
	./${OUT}/main

format: build
	cd ${OUT}; make clangformat

clean:
	rm -rf ${OUT} cmake-build-debug CMakeFiles

# only temporary until we have a proper build thing in place
mpi_tmp: build
	mpiexec ${OUT}/main -n 4096 -m 4096 -i allreduce-butterfly
