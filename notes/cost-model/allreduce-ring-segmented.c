#include <mpi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define TAG_ALLREDUCE -12

/*
 *   ompi_coll_base_allreduce_intra_ring_segmented
 *
 *   Function:       Pipelined ring algorithm for allreduce operation
 *   Accepts:        Same as MPI_Allreduce(), segment size
 *   Returns:        MPI_SUCCESS or error code
 *
 *   Description:    Implements pipelined ring algorithm for allreduce:
 *                   user supplies suggested segment size for the pipelining
 * of reduce operation. The segment size determines the number of phases,
 * np, for the algorithm execution. The message is automatically divided
 * into blocks of approximately  (count / (np * segcount)) elements. At the
 * end of reduction phase, allgather like step is executed. Algorithm
 * requires (np + 1)*(N - 1) steps.
 *
 *   Limitations:    The algorithm DOES NOT preserve order of operations so
 * it can be used only for commutative operations. In addition, algorithm
 * cannot work if the total size is less than size * segment size. Example
 * on 3 nodes with 2 phases Initial state #      0              1 2 [00a]
 * [10a]         [20a] [00b]          [10b]         [20b] [01a] [11a] [21a]
 *        [01b]          [11b]         [21b]
 *        [02a]          [12a]         [22a]
 *        [02b]          [12b]         [22b]
 *
 *        COMPUTATION PHASE 0 (a)
 *         Step 0: rank r sends block ra to rank (r+1) and receives bloc
 * (r-1)a from rank (r-1) [with wraparound]. #     0              1 2 [00a]
 * [00a+10a]       [20a] [00b]          [10b]         [20b] [01a] [11a]
 * [11a+21a] [01b]          [11b]         [21b] [22a+02a]        [12a] [22a]
 *        [02b]          [12b]         [22b]
 *
 *         Step 1: rank r sends block (r-1)a to rank (r+1) and receives bloc
 *                 (r-2)a from rank (r-1) [with wraparound].
 *    #     0              1             2
 *        [00a]        [00a+10a]   [00a+10a+20a]
 *        [00b]          [10b]         [20b]
 *    [11a+21a+01a]      [11a]       [11a+21a]
 *        [01b]          [11b]         [21b]
 *      [22a+02a]    [22a+02a+12a]     [22a]
 *        [02b]          [12b]         [22b]
 *
 *        COMPUTATION PHASE 1 (b)
 *         Step 0: rank r sends block rb to rank (r+1) and receives bloc
 * (r-1)b from rank (r-1) [with wraparound]. #     0              1 2 [00a]
 * [00a+10a]       [20a] [00b]        [00b+10b]       [20b] [01a] [11a]
 * [11a+21a] [01b]          [11b]       [11b+21b] [22a+02a]        [12a]
 * [22a] [22b+02b]        [12b]         [22b]
 *
 *         Step 1: rank r sends block (r-1)b to rank (r+1) and receives bloc
 *                 (r-2)b from rank (r-1) [with wraparound].
 *    #     0              1             2
 *        [00a]        [00a+10a]   [00a+10a+20a]
 *        [00b]          [10b]     [0bb+10b+20b]
 *    [11a+21a+01a]      [11a]       [11a+21a]
 *    [11b+21b+01b]      [11b]         [21b]
 *      [22a+02a]    [22a+02a+12a]     [22a]
 *        [02b]      [22b+01b+12b]     [22b]
 *
 *
 *        DISTRIBUTION PHASE: ring ALLGATHER with ranks shifted by 1 (same
 * as in regular ring algorithm.
 *
 */

#if 0

// P: #procs
// D: #bytes
// C: count = D / 8

// Assuming C * 8 > 2^20 (more than 1MB of data)
const int segcount = segsize / 8; // == 2^17

/* Special case for C less than P * segcount - use regular ring */
if (C < (P * segcount)) {
    return -1;
}

/* Determine the number of phases of the algorithm */
const int num_phases = C / (P * segcount);
const int block_count = C / P;
const int max_segcount = block_count / num_phases;
const int phase_count = block_count / num_phases;

/*
 * num_phases = C / (P * segcount) rounds
 *
 *     o + L + (phase_count * 8 - 1) * G + g
 *          
 *          P - 2 rounds
 *
 *          o + L + (phase_count * 8 - 1) * G + g
 *          phase_count ADDs
 *
 *     phase_count ADDs
 *
 * P rounds
 *
 *     o + L + (block_count * 8 - 1) * G + g
 */

/*
 * ADDs: num_phases * ((P-2) * phase_count + phase_count)
 *      = num_phases * (P - 1) * phase_count
 *      = num_phases * (P - 1) * C / P / num_phases
 *      = C * (P-1) / P
 *      = O(C)
 *
 * LogGP: num_phases * ((o + L + (phase_count * 8 - 1) * G + g) + (P - 2) * (o + L + (phase_count * 8 - 1) * G + g)) + P * (o + L + (block_count * 8 - 1) * G + g)
 *      = num_phases * ((o + L + (C / P / num_phases * 8 - 1) * G + g) + (P - 2) * (o + L + (C / P / num_phases * 8 - 1) * G + g)) + P * (o + L + (C / P * 8 - 1) * G + g)
 *      = C / (P * segcount) * ((o + L + (C / P / (C / (P * segcount)) * 8 - 1) * G + g) + (P - 2) * (o + L + (C / P / (C / (P * segcount)) * 8 - 1) * G + g)) + P * (o + L + (C / P * 8 - 1) * G + g)
 *      = C / (P * segcount) * ((o + L + (segcount * 8 - 1) * G + g) + (P - 2) * (o + L + (segcount * 8 - 1) * G + g)) + P * (o + L + (C / P * 8 - 1) * G + g)
 *      = C / (P * segcount) * ((o + L + (segsize - 1) * G + g) + (P - 2) * (o + L + (segsize - 1) * G + g)) + P * (o + L + (D / P - 1) * G + g)
 *      \in O(C / (P * segcount) * ((o + L + (segsize - 1) * G + g) + (P - 2) * (o + L + (segsize - 1) * G + g)) + P * (o + L + (D / P - 1) * G + g))
 *      = O(C / (P * segcount) * ((o + L + segsize * G + g) + P * (o + L + segsize * G + g)) + P * (o + L + D / P * G + g))
 *      = O(C / (P * segcount) * (P * (o + L + segsize * G + g)) + P * (o + L + g) + D * G)
 *      = O(C / segcount * (o + L + segsize * G + g) + P * (o + L + g) + D * G)
 *      = O(C / segcount * (o + L + g) + D * G + P * (o + L + g) + D * G)
 *      = O(C / segcount * (o + L + g) + P * (o + L + g) + D * G)
 *      = O((C / segcount + P) * (o + L + g) + D * G)
 */

#endif

static MPI_Comm comm;
int allreduce_ring_segmented(const void *sbuf, void *rbuf, int count) {
    uint32_t segsize = 1 << 20;
    int k, recv_from, send_to;
    int inbi;
    char *inbuf[2] = {NULL, NULL};
    ptrdiff_t block_offset;

    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    /* Special case for size == 1 */
    if (1 == size) {
        // ...
        return 0;
    }

    // Assuming count * 8 > 2^20 (more than 1MB of data)
    const int segcount = segsize / 8; // == 2^17

    /* Special case for count less than size * segcount - use regular ring */
    if (count < (size * segcount)) {
        return -1;
    }

    /* Determine the number of phases of the algorithm */
    const int num_phases = count / (size * segcount);

    /* Determine the number of elements per block and corresponding
       block sizes.
       The blocks are divided into "early" and "late" ones:
       blocks 0 .. (split_rank - 1) are "early" and
       blocks (split_rank) .. (size - 1) are "late".
       Early blocks are at most 1 element larger than the late ones.
       Note, these blocks will be split into num_phases segments,
       out of the largest one will have max_segcount elements.
       */
    // early_blockcount and late_blockcount are almost the same (+/- 1)
    const int block_count = count / size;
    const int split_rank = count % size;
    const int max_segcount = block_count / num_phases;


    // early_phase_segcount and late_phase_segcount are almost the same (+/-
    // 1)
    const int phase_count = block_count / num_phases;
    const int split_phase = block_count % num_phases;

    /* Allocate and initialize temporary buffers */
    inbuf[0] = malloc(max_segcount * 8);
    inbuf[1] = malloc(max_segcount * 8);

    memcpy(rbuf, sbuf, 8 * count);
    /*
     * Computation loop: for each phase, repeat ring allreduce computation loop
     */

    MPI_Request reqs[2] = {NULL, NULL};
    // #### num_phases rounds
    for (int phase = 0; phase < num_phases; phase++) {
        char *tmprecv;

        /*
           For each of the remote nodes:
           - post irecv for block (r-1)
           - send block (r)
           To do this, first compute block offset and count, and use block
           offset to compute phase offset.
           - in loop for every step k = 2 .. n
           - post irecv for block (r + n - k) % n
           - wait on block (r + n - k + 1) % n to arrive
           - compute on block (r + n - k + 1) % n
           - send block (r + n - k + 1) % n
           - wait on block (r + 1)
           - compute on block (r + 1)
           - send block (r + 1) to rank (r + 1)
           Note that we must be careful when computing the begining of buffers
           and for send operations and computation we must compute the exact
           block size.
           */
        send_to = (rank + 1) % size;
        recv_from = (rank + size - 1) % size;

        inbi = 0;
        /* Initialize first receive from the neighbor on the left */

        /*
         * #### Irecv max_segcount * 8 bytes
         */
        MPI_Irecv(inbuf[inbi], max_segcount, MPI_DOUBLE, recv_from, TAG_ALLREDUCE, comm, &reqs[inbi]);
        /* Send first block (my block) to the neighbor on the right:
           - compute my block and phase offset
           - send data */
        block_offset =
            ((rank < split_rank) ? (rank * block_count)
                                 : (rank * block_count + split_rank));
        ptrdiff_t phase_offset =
            ((phase < split_phase) ? (phase * phase_count)
                                   : (phase * phase_count + split_phase));
        char *tmpsend = rbuf + (block_offset + phase_offset) * 8;

        /*
         * #### MPI_Send phase_count * 8 bytes
         */
        MPI_Send(tmpsend, phase_count, MPI_DOUBLE, send_to, TAG_ALLREDUCE,
                 comm);

        // #### P - 2 rounds
        for (k = 2; k < size; k++) {
            const int prevblock = (rank + size - k + 1) % size;

            inbi = inbi ^ 0x1;

            /* Post irecv for the current block */
            /*
             * #### Irecv max_segcount * 8 bytes
             */
            MPI_Irecv(inbuf[inbi], max_segcount, MPI_DOUBLE, recv_from,
                      TAG_ALLREDUCE, comm, &reqs[inbi]);

            /* Wait on previous block to arrive */
            /*
             * #### ???
             */
            MPI_Wait(&reqs[inbi ^ 0x1], MPI_STATUS_IGNORE);

            /* Apply operation on previous block: result goes to rbuf
               rbuf[prevblock] = inbuf[inbi ^ 0x1] (op) rbuf[prevblock]
               */
            block_offset = ((prevblock < split_rank)
                                ? (prevblock * block_count)
                                : (prevblock * block_count + split_rank));
            phase_offset =
                ((phase < split_phase) ? (phase * phase_count)
                                       : (phase * phase_count + split_phase));
            tmprecv = rbuf + (block_offset + phase_offset) * 8;

            // #### O(phase_count) ADD
            for (int i = 0; i < phase_count; i++) {
                tmprecv[i] += inbuf[inbi ^ 0x1][i];
            }

            /* send previous block to send_to */
            /*
             * #### MPI_Send phase_count * 8 bytes
             */
            MPI_Send(tmprecv, phase_count, MPI_DOUBLE, send_to, TAG_ALLREDUCE,
                     comm);
        }

        /* Wait on the last block to arrive */
        /*
         * #### ???
         */
        MPI_Wait(&reqs[inbi], MPI_STATUS_IGNORE);

        /* Apply operation on the last block (from neighbor (rank + 1)
           rbuf[rank+1] = inbuf[inbi] (op) rbuf[rank + 1] */
        recv_from = (rank + 1) % size;
        block_offset =
            ((recv_from < split_rank) ? (recv_from * block_count)
                                      : (recv_from * block_count + split_rank));
        phase_offset =
            ((phase < split_phase) ? (phase * phase_count)
                                   : (phase * phase_count + split_phase));
        tmprecv = rbuf + (block_offset + phase_offset) * 8;

        // #### phase_count ADD
        for (int i = 0; i < phase_count; i++) {
            tmprecv[i] += inbuf[inbi][i];
        }
    }

    /* Distribution loop - variation of ring allgather */
    send_to = (rank + 1) % size;
    recv_from = (rank + size - 1) % size;
    // #### P rounds
    for (k = 0; k < size - 1; k++) {
        const int recv_data_from = (rank + size - k) % size;
        const int send_data_from = (rank + 1 + size - k) % size;
        const int send_block_offset =
            ((send_data_from < split_rank)
                 ? (send_data_from * block_count)
                 : (send_data_from * block_count + split_rank));
        const int recv_block_offset =
            ((recv_data_from < split_rank)
                 ? (recv_data_from * block_count)
                 : (recv_data_from * block_count + split_rank));

        char *tmprecv = rbuf + recv_block_offset * 8;
        char *tmpsend = rbuf + send_block_offset * 8;

        /*
         * #### Sendrecv block_count * 8 bytes
         */
        MPI_Sendrecv(tmpsend, block_count, MPI_DOUBLE, send_to, TAG_ALLREDUCE,
                     tmprecv, block_count, MPI_DOUBLE, recv_from, TAG_ALLREDUCE,
                     comm, MPI_STATUS_IGNORE);
    }

    return MPI_SUCCESS;
}
