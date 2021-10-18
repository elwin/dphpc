OUT="build_output"

build:
	CXX=mpic++ cmake -S . -B ${OUT}
	cmake --build ${OUT}

test: build
	cd ${OUT} && ctest

main: build
	./${OUT}/main
