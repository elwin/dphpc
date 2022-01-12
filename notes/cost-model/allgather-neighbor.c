#include <mpi.h>
#include <string.h>
#define MCA_COLL_BASE_TAG_ALLGATHER -10

/*
 * ompi_coll_base_allgather_intra_neighborexchange
 *
 * Function:     allgather using N/2 steps (O(N))
 * Accepts:      Same arguments as MPI_Allgather
 * Returns:      MPI_SUCCESS or error code
 *
 * Description:  Neighbor Exchange algorithm for allgather.
 *               Described by Chen et.al. in
 *               "Performance Evaluation of Allgather Algorithms on
 *                Terascale Linux Cluster with Fast Ethernet",
 *               Proceedings of the Eighth International Conference on
 *               High-Performance Computing inn Asia-Pacific Region
 *               (HPCASIA'05), 2005
 *
 *               Rank r exchanges message with one of its neighbors and
 *               forwards the data further in the next step.
 *
 *               No additional memory requirements.
 *
 * Limitations:  Algorithm works only on even number of processes.
 *               For odd number of processes we switch to ring algorithm.
 *
 * Example on 6 nodes:
 *  Initial state
 *    #     0      1      2      3      4      5
 *         [0]    [ ]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [1]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [2]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [3]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [4]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [ ]    [5]
 *   Step 0:
 *    #     0      1      2      3      4      5
 *         [0]    [0]    [ ]    [ ]    [ ]    [ ]
 *         [1]    [1]    [ ]    [ ]    [ ]    [ ]
 *         [ ]    [ ]    [2]    [2]    [ ]    [ ]
 *         [ ]    [ ]    [3]    [3]    [ ]    [ ]
 *         [ ]    [ ]    [ ]    [ ]    [4]    [4]
 *         [ ]    [ ]    [ ]    [ ]    [5]    [5]
 *   Step 1:
 *    #     0      1      2      3      4      5
 *         [0]    [0]    [0]    [ ]    [ ]    [0]
 *         [1]    [1]    [1]    [ ]    [ ]    [1]
 *         [ ]    [2]    [2]    [2]    [2]    [ ]
 *         [ ]    [3]    [3]    [3]    [3]    [ ]
 *         [4]    [ ]    [ ]    [4]    [4]    [4]
 *         [5]    [ ]    [ ]    [5]    [5]    [5]
 *   Step 2:
 *    #     0      1      2      3      4      5
 *         [0]    [0]    [0]    [0]    [0]    [0]
 *         [1]    [1]    [1]    [1]    [1]    [1]
 *         [2]    [2]    [2]    [2]    [2]    [2]
 *         [3]    [3]    [3]    [3]    [3]    [3]
 *         [4]    [4]    [4]    [4]    [4]    [4]
 *         [5]    [5]    [5]    [5]    [5]    [5]
 */
static MPI_Comm comm;
int allgather_neighbor(const char *sbuf, const int scount, char *rbuf, const int rcount) {
    int rank, size;
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    if (size % 2) {
        // Use ring
        return 0;
    }

    /* Initialization step:
       - if send buffer is not MPI_IN_PLACE, copy send buffer to appropriate
       block of receive buffer
    */
    char *tmprecv = rbuf + rank * rcount * 8;
    memcpy(tmprecv, sbuf, scount * 8);

    int neighbor_right = (rank + 1) % size;
    int neighbor_left = (rank - 1 + size) % size;

    /* Determine neighbors, order in which blocks will arrive, etc. */
    int even_rank = !(rank % 2);
    int neighbor[2];
    int offset_at_step[2];
    int recv_data_from[2];
    if (even_rank) {
        neighbor[0] = neighbor_right;
        neighbor[1] = neighbor_left;
        recv_data_from[0] = rank;
        recv_data_from[1] = rank;
        offset_at_step[0] = (+2);
        offset_at_step[1] = (-2);
    } else {
        neighbor[0] = neighbor_left;
        neighbor[1] = neighbor_right;
        recv_data_from[0] = neighbor[0];
        recv_data_from[1] = neighbor[0];
        offset_at_step[0] = (-2);
        offset_at_step[1] = (+2);
    }

    /* Communication loop:
       - First step is special: exchange a single block with neighbor[0].
       - Rest of the steps:
       update recv_data_from according to offset, and
       exchange two blocks with appropriate neighbor.
       the send location becomes previous receve location.
    */
    tmprecv = rbuf + neighbor[0] * rcount * 8;
    char *tmpsend = rbuf + rank * rcount * 8;
    /*
     * Sendrecv rcount * 8 bytes
     */
    MPI_Sendrecv(tmpsend, rcount, MPI_DOUBLE, neighbor[0],
                 MCA_COLL_BASE_TAG_ALLGATHER, tmprecv, rcount, MPI_DOUBLE,
                 neighbor[0], MCA_COLL_BASE_TAG_ALLGATHER, comm,
                 MPI_STATUS_IGNORE);

    /* Determine initial sending location */
    int send_data_from = even_rank ? rank : recv_data_from[0];

    for (int i = 1; i < (size / 2); i++) {
        const int i_parity = i % 2;
        recv_data_from[i_parity] =
            (recv_data_from[i_parity] + offset_at_step[i_parity] + size) % size;

        tmprecv = rbuf + recv_data_from[i_parity] * rcount * 8;
        tmpsend = rbuf + send_data_from * rcount * 8;

        /*
         * Sendrecv 2 * rcount * 8 bytes
         */
        MPI_Sendrecv(tmpsend, 2 * rcount, MPI_DOUBLE, neighbor[i_parity],
                     MCA_COLL_BASE_TAG_ALLGATHER, tmprecv, 2 * rcount,
                     MPI_DOUBLE, neighbor[i_parity],
                     MCA_COLL_BASE_TAG_ALLGATHER, comm, MPI_STATUS_IGNORE);

        send_data_from = recv_data_from[i_parity];
    }

    return 0;
}
