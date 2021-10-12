OUT="build_output"

build:
	cmake -S . -B ${OUT}
	cmake --build ${OUT}

test: build
	cd ${OUT} && ctest

main: build
	./build/main