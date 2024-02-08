#include <iostream>
#include <cctype>
#include <random>
#include <vector>

#include "mpi.h"

using namespace std;
using u32 = uint_least32_t;
using engine = mt19937;

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);
	int rank, size;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// random generation
	random_device os_seed;
	const u32 seed = os_seed();
	engine generator(seed);

	uniform_int_distribution<u32> distribute(0, 1);
	int color = distribute(generator);

	cout << "Process " << rank << " has n = " << color << endl;

	// new communicator
	MPI_Comm nc;
	MPI_Comm_split(MPI_COMM_WORLD, color, rank, &nc);

	if (color == 1) {
		uniform_int_distribution<u32> distribute(1, 1000);
		int a = distribute(generator);

		int box[2];
		box[0] = rank;
		box[1] = a;

		// polling the number of processes in the communication area
		int nc_size;
		MPI_Comm_size(nc, &nc_size);

		vector<int> received_data(nc_size * 2);
		MPI_Allgather(&box[0], 2, MPI_INT, &received_data[0], 2, MPI_INT, nc);

		for (int i = nc_size - 1; i >= 0; --i) {
			int sender_rank = received_data[i * 2];
			int sender_a = received_data[i * 2 + 1];

			cout << "Process " << rank << " with n = " << color << " received A from " << sender_rank << " process: " << sender_a << endl;
		}
	}

	MPI_Finalize();

	return 0;
}
