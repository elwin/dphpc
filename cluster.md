# Running on the cluster

First, before accessing the cluster, one must be inside the ETH network. This can easily be achieved by using the ETH
VPN. As soon as we are inside the network, we can connect to the Euler cluster via SSH: `<nethz username>@euler.ethz.ch`

## Checking out the codebase

As git is available on the cluster, you can simply clone this repository into you personal home directory on the
cluster: `git clone https://github.com/elwin/dphpc`

> You can also send [@elwin](mailto:elwin.stephan@gmail.com) a copy of your public key on your Euler account and he'll add it as a deploy key to the repository. Afterwards, you can `git clone git@github.com:elwin/dphpc.git` without having to do any further authorization :crystal_ball:

## Preparations

### Environment

Before we can build and run our code, we have to prepare the environment to use the correct versions of our build tools.
To switch to the new software stack and load the required modules, run:

```shell
source ~/dphpc/scripts/euler/init.sh
```

Or check out what the file does, you can of course also run it manually in your shell.

Then you should be able to build the project with `make <target>` after `cd`'ing into the project folder.

### SSHing

> Some scripts assume the the SSH alias `euler` to be set (i.e. `ssh euler` should log you into euler). This step is not necessary for all scripts, you'll see soon enough when something fails when omitting this step.

Create an SSH key for uploading to Euler.

```shell
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_euler
```

Upload the public key to the euler cluster.

```shell
ssh-copy-id -i $HOME/.ssh/id_ed25519_euler.pub username@euler.ethz.ch
```

Create an SSH config file (or extend the current one if it already exists) under `~/.ssh/config` on your local computer,
and put the following content there:

```
Host euler
    HostName euler.ethz.ch
    User <eth-username>
    IdentityFile ~/.ssh/id_ed25519_euler
```

You can now ssh into the euler cluster using `ssh euler`.

## Run jobs

```shell
# Directly run it on the local node
mpirun -np <number of processes> <binary>
# So for example:
mpirun -np 4 ./main -n 5 -m 5 -i allreduce

# You can also run it as a job on the cluster
bsub -n <number of processes> mpirun -np <number of processes> <binary>
# Example:
bsub -n 4 mpirun -np 4 ./main -n 5 -m 5 -i allreduce
```

## Running Benchmark File

### Preparations

### Submit Jobs