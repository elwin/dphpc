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

	// Use the rank as the seed
	std::mt19937_64 gen{(uint64_t)rank};
	std::uniform_real_distribution<double> dist(-1.0, 1.0);

	printf("Starting rank=%d size=%d\n", rank, numprocs);

	auto vector = std::make_unique<double[]>(N + M);

	printf("A_%d = [", rank);
	for (int i = 0; i < N; i++) {
		vector[i] = dist(gen);
		printf("%f ", vector[i]);
	}
	printf("]\n");

	printf("B_%d = [", rank);
	for (int i = N; i < M; i++) {
		vector[i] = dist(gen);
		printf("%f ", vector[i]);
	}
	printf("]\n");

	double t = -MPI_Wtime();
	// TODO: Does this work with MPI_Allgather?
	auto vectors = (double*)malloc((numprocs * (N + M) * sizeof(double)));

	// Use MPI_Allgather to collect all vectors on all nodes into 'vectors'
	MPI_Allgather(vector.get(), 1, MPI_DOUBLE, vectors, 1, MPI_DOUBLE, MPI_COMM_WORLD);

    // Calculate sum of products on all nodes
    auto S = std::make_unique<double[]>(N * M);
	for (int i = 0; i < numprocs; i++) {
        auto A = std::make_unique<double[]>(N);
        auto B = std::make_unique<double[]>(M);

        for (int j = 0; j < N+M; j++) {
            if (j < N) {
                A[i] = vectors[i * (N + M) + j];
            } else {
                B[i] = vectors[i * (N + M) + j];
            }
        }

        auto T = std::make_unique<double[]>(N * M);
        for (int k = 0; k < N; k++) {
            for (int l = 0; l < M; l++) {
                T[k * N + l] = A[k] * B[l];
            }
        }

        // TODO: matrix addition
        S += T
	}

	t += MPI_Wtime();

    // Print result only once
	if (rank == 0) {
		printf("result:\n");
		for (int i = 0; i < N; i++) {
			for (int j = 0; j < M; j++) {
				printf("%f ", S[i * N + j]);
			}
			printf("\n");
		}
		printf("time: %f\n", t);
	}
	MPI_Finalize();
}
