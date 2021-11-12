# Algorithms
(information taken from open-mpi repo)

From `coll_tuned_allgather_decision.c`:

```c
static const mca_base_var_enum_value_t allgather_algorithms[] = {
    {0, "ignore"},
    {1, "linear"},
    {2, "bruck"},
    {3, "recursive_doubling"},
    {4, "ring"},
    {5, "neighbor"},
    {6, "two_proc"},
    {7, "sparbit"},
    {0, NULL}
};
```

A value of 0 means that the decision tree is used.

## Implementations

All algorithms are defined in `coll_base_allgather.c`.

### Linear

`ompi_coll_base_allgather_intra_basic_linear`

Gather (to root) and broadcast

### Bruck

```
ompi_coll_base_allgather_intra_bruck

Function:     allgather using O(log(N)) steps.
Accepts:      Same arguments as MPI_Allgather
Returns:      MPI_SUCCESS or error code

Description:  Variation to All-to-all algorithm described by Bruck et al.in
              "Efficient Algorithms for All-to-all Communications
               in Multiport Message-Passing Systems"
Memory requirements:  non-zero ranks require shift buffer to perform final
              step in the algorithm.

Example on 6 nodes:
  Initialization: everyone has its own buffer at location 0 in rbuf
                  This means if user specified MPI_IN_PLACE for sendbuf
                  we must copy our block from recvbuf to begining!
   #     0      1      2      3      4      5
        [0]    [1]    [2]    [3]    [4]    [5]
  Step 0: send message to (rank - 2^0), receive message from (rank + 2^0)
   #     0      1      2      3      4      5
        [0]    [1]    [2]    [3]    [4]    [5]
        [1]    [2]    [3]    [4]    [5]    [0]
  Step 1: send message to (rank - 2^1), receive message from (rank + 2^1)
          message contains all blocks from location 0 to 2^1*block size
   #     0      1      2      3      4      5
        [0]    [1]    [2]    [3]    [4]    [5]
        [1]    [2]    [3]    [4]    [5]    [0]
        [2]    [3]    [4]    [5]    [0]    [1]
        [3]    [4]    [5]    [0]    [1]    [2]
  Step 2: send message to (rank - 2^2), receive message from (rank + 2^2)
          message size is "all remaining blocks"
   #     0      1      2      3      4      5
        [0]    [1]    [2]    [3]    [4]    [5]
        [1]    [2]    [3]    [4]    [5]    [0]
        [2]    [3]    [4]    [5]    [0]    [1]
        [3]    [4]    [5]    [0]    [1]    [2]
        [4]    [5]    [0]    [1]    [2]    [3]
        [5]    [0]    [1]    [2]    [3]    [4]
   Finalization: Do a local shift to get data in correct place
   #     0      1      2      3      4      5
        [0]    [0]    [0]    [0]    [0]    [0]
        [1]    [1]    [1]    [1]    [1]    [1]
        [2]    [2]    [2]    [2]    [2]    [2]
        [3]    [3]    [3]    [3]    [3]    [3]
        [4]    [4]    [4]    [4]    [4]    [4]
        [5]    [5]    [5]    [5]    [5]    [5]
```

Every process holds a vector of size `n`.

Every round, each process sends its entire buffer (`2^t * n`) to `rank - 2^t` (round `t`) and receives from `rank + 2^t`.

Each process' buffer doubles each round and the buffer size sent also doubles each round.

### Recursive Doubling

```
ompi_coll_base_allgather_intra_recursivedoubling

Function:     allgather using O(log(N)) steps.
Accepts:      Same arguments as MPI_Allgather
Returns:      MPI_SUCCESS or error code

Description:  Recursive doubling algorithm for MPI_Allgather implementation.
              This algorithm is used in MPICH-2 for small- and medium-sized
              messages on power-of-two processes.

Limitation:   Current implementation only works on power-of-two number of
              processes.
              In case this algorithm is invoked on non-power-of-two
              processes, Bruck algorithm will be invoked.

Memory requirements:
              No additional memory requirements beyond user-supplied buffers.

Example on 4 nodes:
  Initialization: everyone has its own buffer at location rank in rbuf
   #     0      1      2      3
        [0]    [ ]    [ ]    [ ]
        [ ]    [1]    [ ]    [ ]
        [ ]    [ ]    [2]    [ ]
        [ ]    [ ]    [ ]    [3]
  Step 0: exchange data with (rank ^ 2^0)
   #     0      1      2      3
        [0]    [0]    [ ]    [ ]
        [1]    [1]    [ ]    [ ]
        [ ]    [ ]    [2]    [2]
        [ ]    [ ]    [3]    [3]
  Step 1: exchange data with (rank ^ 2^1) (if you can)
   #     0      1      2      3
        [0]    [0]    [0]    [0]
        [1]    [1]    [1]    [1]
        [2]    [2]    [2]    [2]
        [3]    [3]    [3]    [3]
```

In round `t` exchange all your data (from `2^t` processes) with process
`rank XOR 2^t`.

Another way to look at this is that all processes are organized in a binary
tree with the processes at the leaves.

Each round you exchange your buffer contents with the corresponding process in your sibling node and move up to your parent node.

```
   ┌───┴───┐
 ┌─┴─┐   ┌─┴─┐
┌┴┐ ┌┴┐ ┌┴┐ ┌┴┐
0 1 2 3 4 5 6 7
```

For example for process 6. First it exchanges with 7, then 4, then 2.

The non-leaf nodes in the tree always have all data from all children.

### Ring

```
ompi_coll_base_allgather_intra_ring

Function:     allgather using O(N) steps.
Accepts:      Same arguments as MPI_Allgather
Returns:      MPI_SUCCESS or error code

Description:  Ring algorithm for all gather.
              At every step i, rank r receives message from rank (r - 1)
              containing data from rank (r - i - 1) and sends message to rank
              (r + 1) containing data from rank (r - i), with wrap arounds.
Memory requirements:
              No additional memory requirements.
```

Each round, each process sends the data received in the previous round to the next process.

### Neighbor

```
ompi_coll_base_allgather_intra_neighborexchange

Function:     allgather using N/2 steps (O(N))
Accepts:      Same arguments as MPI_Allgather
Returns:      MPI_SUCCESS or error code

Description:  Neighbor Exchange algorithm for allgather.
              Described by Chen et.al. in
              "Performance Evaluation of Allgather Algorithms on
               Terascale Linux Cluster with Fast Ethernet",
              Proceedings of the Eighth International Conference on
              High-Performance Computing inn Asia-Pacific Region
              (HPCASIA'05), 2005

              Rank r exchanges message with one of its neighbors and
              forwards the data further in the next step.

              No additional memory requirements.

Limitations:  Algorithm works only on even number of processes.
              For odd number of processes we switch to ring algorithm.

Example on 6 nodes:
 Initial state
   #     0      1      2      3      4      5
        [0]    [ ]    [ ]    [ ]    [ ]    [ ]
        [ ]    [1]    [ ]    [ ]    [ ]    [ ]
        [ ]    [ ]    [2]    [ ]    [ ]    [ ]
        [ ]    [ ]    [ ]    [3]    [ ]    [ ]
        [ ]    [ ]    [ ]    [ ]    [4]    [ ]
        [ ]    [ ]    [ ]    [ ]    [ ]    [5]
  Step 0:
   #     0 <──> 1      2 <──> 3      4 <──> 5
        [0]    [0]    [ ]    [ ]    [ ]    [ ]
        [1]    [1]    [ ]    [ ]    [ ]    [ ]
        [ ]    [ ]    [2]    [2]    [ ]    [ ]
        [ ]    [ ]    [3]    [3]    [ ]    [ ]
        [ ]    [ ]    [ ]    [ ]    [4]    [4]
        [ ]    [ ]    [ ]    [ ]    [5]    [5]
  Step 1:
   #  ─> 0      1 <──> 2      3 <──> 4      5 <─
        [0]    [0]    [0]    [ ]    [ ]    [0]
        [1]    [1]    [1]    [ ]    [ ]    [1]
        [ ]    [2]    [2]    [2]    [2]    [ ]
        [ ]    [3]    [3]    [3]    [3]    [ ]
        [4]    [ ]    [ ]    [4]    [4]    [4]
        [5]    [ ]    [ ]    [5]    [5]    [5]
  Step 2:
   #     0 <──> 1      2 <──> 3      4 <──> 5
        [0]    [0]    [0]    [0]    [0]    [0]
        [1]    [1]    [1]    [1]    [1]    [1]
        [2]    [2]    [2]    [2]    [2]    [2]
        [3]    [3]    [3]    [3]    [3]    [3]
        [4]    [4]    [4]    [4]    [4]    [4]
        [5]    [5]    [5]    [5]    [5]    [5]

```

### Two Proc

`ompi_coll_base_allgather_intra_two_procs`

Special case for two processes. They just exchange their data.

### Sparbit

```
ompi_coll_base_allgather_intra_sparbit

Function:     allgather using O(log(N)) steps.
Accepts:      Same arguments as MPI_Allgather
Returns:      MPI_SUCCESS or error code

Description: Proposal of an allgather algorithm similar to Bruck but with inverted distances
             and non-decreasing exchanged data sizes. Described in "Sparbit: a new
             logarithmic-cost and data locality-aware MPI Allgather algorithm".

Memory requirements:
             Additional memory for N requests.

Example on 6 nodes, with l representing the highest power of two smaller than N, in this case l =
4 (more details can be found on the paper):
 Initial state
   #     0      1      2      3      4      5
        [0]    [ ]    [ ]    [ ]    [ ]    [ ]
        [ ]    [1]    [ ]    [ ]    [ ]    [ ]
        [ ]    [ ]    [2]    [ ]    [ ]    [ ]
        [ ]    [ ]    [ ]    [3]    [ ]    [ ]
        [ ]    [ ]    [ ]    [ ]    [4]    [ ]
        [ ]    [ ]    [ ]    [ ]    [ ]    [5]
  Step 0: Each process sends its own block to process r + l and receives another from r - l.
   #     0      1      2      3      4      5
        [0]    [ ]    [ ]    [ ]    [0]    [ ]
        [ ]    [1]    [ ]    [ ]    [ ]    [1]
        [2]    [ ]    [2]    [ ]    [ ]    [ ]
        [ ]    [3]    [ ]    [3]    [ ]    [ ]
        [ ]    [ ]    [4]    [ ]    [4]    [ ]
        [ ]    [ ]    [ ]    [5]    [ ]    [5]
  Step 1: Each process sends its own block to process r + l/2 and receives another from r - l/2.
  The block received on the previous step is ignored to avoid a future double-write.
   #     0      1      2      3      4      5
        [0]    [ ]    [0]    [ ]    [0]    [ ]
        [ ]    [1]    [ ]    [1]    [ ]    [1]
        [2]    [ ]    [2]    [ ]    [2]    [ ]
        [ ]    [3]    [ ]    [3]    [ ]    [3]
        [4]    [ ]    [4]    [ ]    [4]    [ ]
        [ ]    [5]    [ ]    [5]    [ ]    [5]
  Step 1: Each process sends all the data it has (3 blocks) to process r + l/4 and similarly
  receives all the data from process r - l/4.
   #     0      1      2      3      4      5
        [0]    [0]    [0]    [0]    [0]    [0]
        [1]    [1]    [1]    [1]    [1]    [1]
        [2]    [2]    [2]    [2]    [2]    [2]
        [3]    [3]    [3]    [3]    [3]    [3]
        [4]    [4]    [4]    [4]    [4]    [4]
        [5]    [5]    [5]    [5]    [5]    [5]

```

## Decision Tree

It seems the `linear` and `sparbit` algos are never used in the decision tree.

`communicator_size` is the number of processes.
`total_dsize` is total number of bytes to be transferred (summed over all
processes).

```c
if (communicator_size == 2) {
    alg = 6; // two_proc
} else if (communicator_size < 32) {
    alg = 3; // recursive_doubling
} else if (communicator_size < 64) {
    if (total_dsize < 1024) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 65536) {
        alg = 5; // neighbor
    } else {
        alg = 4; // ring
    }
} else if (communicator_size < 128) {
    if (total_dsize < 512) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 65536) {
        alg = 5; // neighbor
    } else {
        alg = 4; // ring
    }
} else if (communicator_size < 256) {
    if (total_dsize < 512) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 131072) {
        alg = 5; // neighbor
    } else if (total_dsize < 524288) {
        alg = 4; // ring
    } else if (total_dsize < 1048576) {
        alg = 5; // neighbor
    } else {
        alg = 4; // ring
    }
} else if (communicator_size < 512) {
    if (total_dsize < 32) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 128) {
        alg = 2; // bruck
    } else if (total_dsize < 1024) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 131072) {
        alg = 5; // neighbor
    } else if (total_dsize < 524288) {
        alg = 4; // ring
    } else if (total_dsize < 1048576) {
        alg = 5; // neighbor
    } else {
        alg = 4; // ring
    }
} else if (communicator_size < 1024) {
    if (total_dsize < 64) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 256) {
        alg = 2; // bruck
    } else if (total_dsize < 2048) {
        alg = 3; // recursive_doubling
    } else {
        alg = 5; // neighbor
    }
} else if (communicator_size < 2048) {
    if (total_dsize < 4) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 8) {
        alg = 2; // bruck
    } else if (total_dsize < 16) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 32) {
        alg = 2; // bruck
    } else if (total_dsize < 256) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 512) {
        alg = 2; // bruck
    } else if (total_dsize < 4096) {
        alg = 3; // recursive_doubling
    } else {
        alg = 5; // neighbor
    }
} else if (communicator_size < 4096) {
    if (total_dsize < 32) {
        alg = 2; // bruck
    } else if (total_dsize < 128) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 512) {
        alg = 2; // bruck
    } else if (total_dsize < 4096) {
        alg = 3; // recursive_doubling
    } else {
        alg = 5; // neighbor
    }
} else {
    if (total_dsize < 2) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 8) {
        alg = 2; // bruck
    } else if (total_dsize < 16) {
        alg = 3; // recursive_doubling
    } else if (total_dsize < 512) {
        alg = 2; // bruck
    } else if (total_dsize < 4096) {
        alg = 3; // recursive_doubling
    } else {
        alg = 5; // neighbor
    }
}
```
