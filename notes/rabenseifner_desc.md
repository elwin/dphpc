# Rabenseifner's Algorithm
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

## Limitations:
 *   count >= 2^{\floor{\log_2 p}}
 *   commutative operations only
 *   intra-communicators only

## Memory requirements (per process):
 *   count * typesize + 4 * \log_2(p) * sizeof(int) = O(count)
 *   Schedule length (rounds): O(\log(p))
