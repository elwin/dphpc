# Algorithms
(information taken from open-mpi repo)
## basic_linear
reduce to root node then broadcast.

## Non-Overlapping
ompi_coll_base_allreduce_intra_nonoverlapping

This function just calls a reduce followed by a broadcast
both called functions are base but they complete sequentially,
i.e. no additional overlapping
meaning if the number of segments used is greater than the topo depth
then once the first segment of data is fully 'reduced' it is not broadcast
while the reduce continues (cost = cost-reduce + cost-bcast + decision x 3)

## Recursive Doubling
only supports power of two \#processes
other processes just send data to processes involved in exchanging data.

## Ring Algorithm
ompi_coll_base_allreduce_intra_ring

Function:       Ring algorithm for allreduce operation
Accepts:        Same as MPI_Allreduce()
Returns:        MPI_SUCCESS or error code

Description:    Implements ring algorithm for allreduce: the message is
                automatically segmented to segment of size M/N.
                Algorithm requires 2*N - 1 steps.

Limitations:    The algorithm DOES NOT preserve order of operations so it
                can be used only for commutative operations.
                In addition, algorithm cannot work if the total count is
                less than size.
      Example on 5 nodes:
      Initial state
\#      0              1             2              3             4
     [00]           [10]          [20]           [30]           [40]
     [01]           [11]          [21]           [31]           [41]
     [02]           [12]          [22]           [32]           [42]
     [03]           [13]          [23]           [33]           [43]
     [04]           [14]          [24]           [34]           [44]

     COMPUTATION PHASE
      Step 0: rank r sends block r to rank (r+1) and receives bloc (r-1)
              from rank (r-1) [with wraparound].
 \#     0              1             2              3             4
     [00]          [00+10]        [20]           [30]           [40]
     [01]           [11]         [11+21]         [31]           [41]
     [02]           [12]          [22]          [22+32]         [42]
     [03]           [13]          [23]           [33]         [33+43]
   [44+04]          [14]          [24]           [34]           [44]

      Step 1: rank r sends block (r-1) to rank (r+1) and receives bloc
              (r-2) from rank (r-1) [with wraparound].
 \#      0              1             2              3             4
      [00]          [00+10]     [01+10+20]        [30]           [40]
      [01]           [11]         [11+21]      [11+21+31]        [41]
      [02]           [12]          [22]          [22+32]      [22+32+42]
   [33+43+03]        [13]          [23]           [33]         [33+43]
     [44+04]       [44+04+14]       [24]           [34]           [44]

      Step 2: rank r sends block (r-2) to rank (r+1) and receives bloc
              (r-2) from rank (r-1) [with wraparound].
 \#      0              1             2              3             4
      [00]          [00+10]     [01+10+20]    [01+10+20+30]      [40]
      [01]           [11]         [11+21]      [11+21+31]    [11+21+31+41]
  [22+32+42+02]      [12]          [22]          [22+32]      [22+32+42]
   [33+43+03]    [33+43+03+13]     [23]           [33]         [33+43]
     [44+04]       [44+04+14]  [44+04+14+24]      [34]           [44]

      Step 3: rank r sends block (r-3) to rank (r+1) and receives bloc
              (r-3) from rank (r-1) [with wraparound].
 \#      0              1             2              3             4
      [00]          [00+10]     [01+10+20]    [01+10+20+30]     [FULL]
     [FULL]           [11]        [11+21]      [11+21+31]    [11+21+31+41]
  [22+32+42+02]     [FULL]          [22]         [22+32]      [22+32+42]
   [33+43+03]    [33+43+03+13]     [FULL]          [33]         [33+43]
     [44+04]       [44+04+14]  [44+04+14+24]      [FULL]         [44]

     DISTRIBUTION PHASE: ring ALLGATHER with ranks shifted by 1.

## Segmented Ring
(todo: not found implementation yet)

## Rabenseifner's Algorithm
(quote from ompi/mca/coll/libnbc/nbc_iallreduce.c)
This algorithm is a combination of a reduce-scatter implemented with
recursive vector halving and recursive distance doubling, followed either
by an allgather implemented with recursive doubling.

Step 1. If the number of processes is not a power of two, reduce it to
the nearest lower power of two (p' = 2^{\floor{\log_2 p}})
by removing r = p - p' extra processes as follows. In the first 2r processes
(ranks 0 to 2r - 1), all the even ranks send the second half of the input
vector to their right neighbor (rank + 1), and all the odd ranks send
the first half of the input vector to their left neighbor (rank - 1).
The even ranks compute the reduction on the first half of the vector and
the odd ranks compute the reduction on the second half. The odd ranks then
send the result to their left neighbors (the even ranks). As a result,
the even ranks among the first 2r processes now contain the reduction with
the input vector on their right neighbors (the odd ranks). These odd ranks
do not participate in the rest of the algorithm, which leaves behind
a power-of-two number of processes. The first r even-ranked processes and
the last p - 2r processes are now renumbered from 0 to p' - 1.

Step 2. The remaining processes now perform a reduce-scatter by using
recursive vector halving and recursive distance doubling. The even-ranked
processes send the second half of their buffer to rank + 1 and the odd-ranked
processes send the first half of their buffer to rank - 1. All processes
then compute the reduction between the local buffer and the received buffer.
In the next log_2(p') - 1 steps, the buffers are recursively halved, and the
distance is doubled. At the end, each of the p' processes has 1 / p' of the
total reduction result.

Step 3. An allgather is performed by using recursive vector doubling and
distance halving. All exchanges are executed in reverse order relative
to recursive doubling on previous step. If the number of processes is not
a power of two, the total result vector must be sent to the r processes
that were removed in the first step.

### Limitations:
 *   count >= 2^{\floor{\log_2 p}}
 *   commutative operations only
 *   intra-communicators only

### Memory requirements (per process):
 *   count * typesize + 4 * \log_2(p) * sizeof(int) = O(count)
 *   Schedule length (rounds): O(\log(p))

# Runtime/Cost-Functions
taken from: https://www.mcs.anl.gov/~thakur/papers/ijhpca-coll.pdf
gamma := computation cost per byte for reduction operation locally
beta := transfer-time per byte
alpha := latency

T(recursive_doubling) = lg p * alpha + log p * n * beta + log p * n * gamma
T(ring) = (p-1)*alpha + (p-1)/p*n*beta
T(rabenseifner) = 2* lg p * alpha + 2 * (p-1)/p * n * beta + (p-1)/p * n * gamma

# Decision tree
Assuming that the operation used in allreduce is commutative we have following decision tree
```
if (communicator_size < 4) 
            if (total_dsize < 8) 
                alg = ring
             elif (total_dsize < 4096) 
                alg = recursive_doubling
             elif (total_dsize < 8192) 
                alg = ring
             elif (total_dsize < 16384) 
                alg = recursive_doubling
             elif (total_dsize < 65536) 
                alg = ring
             elif (total_dsize < 262144) 
                alg = segmented_ring
             else 
                alg = rabenseifner
         elif (communicator_size < 8) 
            if (total_dsize < 16) 
                alg = ring
             elif (total_dsize < 8192) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         elif (communicator_size < 16) 
            if (total_dsize < 8192) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         elif (communicator_size < 32) 
            if (total_dsize < 64) 
                alg = segmented_ring
             elif (total_dsize < 4096) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         elif (communicator_size < 64) 
            if (total_dsize < 128) 
                alg = segmented_ring
             else 
                alg = rabenseifner
         elif (communicator_size < 128) 
            if (total_dsize < 262144) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         elif (communicator_size < 256) 
            if (total_dsize < 131072) 
                alg = nonoverlapping
             elif (total_dsize < 262144) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         elif (communicator_size < 512) 
            if (total_dsize < 4096) 
                alg = nonoverlapping
             else 
                alg = rabenseifner
         elif (communicator_size < 2048) 
            if (total_dsize < 2048) 
                alg = nonoverlapping
             elif (total_dsize < 16384) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         elif (communicator_size < 4096) 
            if (total_dsize < 2048) 
                alg = nonoverlapping
             elif (total_dsize < 4096) 
                alg = segmented_ring
             elif (total_dsize < 16384) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
         else 
            if (total_dsize < 2048) 
                alg = nonoverlapping
             elif (total_dsize < 16384) 
                alg = segmented_ring
             elif (total_dsize < 32768) 
                alg = recursive_doubling
             else 
                alg = rabenseifner
```
