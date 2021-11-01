from benchmark import *


def main():
    if False in [EulerRunner().verify(r) for r in repetitions]:
        exit(1)


if __name__ == '__main__':
    main()
