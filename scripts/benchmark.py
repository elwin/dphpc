from lib import *


def main():
    # scheduler = Scheduler(DryRun())
    # with open("xxxx", "a") as f:
    #     scheduler = Scheduler(LocalRunner(binary_path="code/build_output/main", output=f))
    #     scheduler.register(config=Configuration(1, 1, 1, "allgather"))
    #     scheduler.run()

    # scheduler = Scheduler(DryRun())
    scheduler = Scheduler(EulerRunner())
    scheduler.register(config=configs)
    scheduler.run()


if __name__ == '__main__':
    main()
