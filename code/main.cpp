#include <mpi.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <vector>

static constexpr int N = 3;
static constexpr int M = 3;

int main(int argc, char* argv[]) {
	int rank;
	int numprocs;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	bool is_root = rank == 0;

	// Use the rank as the seed
	std::mt19937_64 gen{(uint64_t)rank};
	std::uniform_real_distribution<double> dist(-1.0, 1.0);

	printf("Starting rank=%d size=%d\n", rank, numprocs);

	auto A = std::make_unique<double[]>(N);
	auto B = std::make_unique<double[]>(M);

	printf("A_%d = [", rank);
	for (int i = 0; i < N; i++) {
		A[i] = dist(gen);
		printf("%f ", A[i]);
	}
	printf("]\n");

	printf("B_%d = [", rank);
	for (int i = 0; i < M; i++) {
		B[i] = dist(gen);
		printf("%f ", B[i]);
	}
	printf("]\n");

	double t = -MPI_Wtime();
	auto G = std::make_unique<double[]>(N * M);

//	for (int i = 0; i < N; i++) {
//		for (int j = 0; j < M; j++) {
//			G[i * N + j] = A[i] * B[j];
//		}
//	}
//
//	auto result = std::make_unique<double[]>(N * M);
//	MPI_Allreduce(G.get(), result.get(), N * M, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

/*  Distribute vectors to all nodes and then compute the whole SOP on each node
 *
 *  1. Maybe use MPI_SCATTER to distribute the vectors
 *  2. Cacluate SOP on each node (how?)
 *  3. Gather results (and check if they all computed the same?)
 * */

	t += MPI_Wtime();

	if (is_root) {
		printf("result:\n");
		for (int i = 0; i < N; i++) {
			for (int j = 0; j < M; j++) {
				printf("%f ", result[i * N + j]);
			}
			printf("\n");
		}
		printf("time: %f\n", t);
	}
	MPI_Finalize();
}
