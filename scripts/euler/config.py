import pathlib

binary_path = "code/build_output/main"
bb_output_path = "results/tmp/raw"
parsed_output_path = "results/tmp/parsed"
repetitions = range(0, 3)

[
    pathlib.Path(path).mkdir(parents=True, exist_ok=True)
    for path in [bb_output_path, parsed_output_path]
]
