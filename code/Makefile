# cannot be named "build", otherwise clashes with the target
# and never re-builds once created
OUT="build_output"
IMPLEMENTATIONS = allreduce allreduce-butterfly allreduce-ring allgather allgather-async allreduce-rabenseifner rabenseifner-gather g-rabenseifner-allgather g-rabenseifner-allgather-scatter bruck-async

build:
	cmake -S . -B ${OUT}
	cmake --build ${OUT}

test: build
	cd ${OUT} && ctest

format: build
	cd ${OUT}; make clangformat

clean:
	rm -rf ${OUT} cmake-build-debug CMakeFiles

main: build
	mpiexec ${OUT}/main -n 4096 -m 4096 -i g-rabenseifner-allgather

all: build
	for impl in $(IMPLEMENTATIONS); do \
		mpiexec ${OUT}/main -n 4096 -m 4096 -i $$impl; \
	done
