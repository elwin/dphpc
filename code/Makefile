OUT="build"

build:
	CXX=mpic++ cmake -S . -B ${OUT}
	cmake --build ${OUT}

test: build
	cd ${OUT} && ctest

main: build
	./${OUT}/main
