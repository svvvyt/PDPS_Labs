#include <iostream>
#include "mpi.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // double start = MPI_Wtime();

    int data_size = 3 * size;
    int *data_array = new int[data_size];
    int recvBuff[3];

    for (int i = 0; i < data_size; i++) {
            data_array[i] = rand() % 100;
    }

    MPI_Scatter(data_array, 3, MPI_INT, recvBuff, 3, MPI_INT, 0, MPI_COMM_WORLD);

    printf("Array by process %d: ", rank);
    for (int i = 0; i < 3; i++) {
        printf("%d ", recvBuff[i]);
    }
    printf("\n");

    // double deltaTime = MPI_Wtime() - start;
    // printf("deltaTime: %f", deltaTime);

    MPI_Finalize();

    return 0;
}