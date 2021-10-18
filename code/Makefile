# cannot be named "build", otherwise clashes with the target
# and never re-builds once created
OUT="build_output"

build:
	CXX=mpic++ cmake -S . -B ${OUT}
	cmake --build ${OUT}

test: build
	cd ${OUT} && ctest

main: build
	./${OUT}/main

format: build
	cd build; make clangformat
