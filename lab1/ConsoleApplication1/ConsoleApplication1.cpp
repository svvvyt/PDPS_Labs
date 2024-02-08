#include <iostream>
#include <mpi.h>

using namespace std;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int originalArray[] = { 1, 2, 3, 4, 5 };
    const int arraySize = sizeof(originalArray) / sizeof(int);
    int shiftedArray[arraySize];

    MPI_Status status;

    double start = MPI_Wtime();

    if (size < 2) {
        cout << "This program requires at least 2 processes to run." << endl;
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        cout << "Original array: [";
        for (int i = 0; i < arraySize; i++) {
            cout << originalArray[i] << " ";
        }
        cout << "]" << endl;

        // Send the original array to process 1
        MPI_Send(originalArray, arraySize, MPI_INT, 1, 0, MPI_COMM_WORLD);

        // Receive the shifted array from the last process
        MPI_Recv(shiftedArray, arraySize, MPI_INT, size - 1, 0, MPI_COMM_WORLD, &status);

        cout << "Resulted array: [";
        for (int i = 0; i < arraySize; i++) {
            cout << shiftedArray[i] << " ";
        }
        cout << "]" << endl;
    }
    else {
        // Receive the array from the previous process
        MPI_Recv(shiftedArray, arraySize, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);

        cout << "Process " << rank << " received array: [";
        for (int i = 0; i < arraySize; i++) {
            cout << shiftedArray[i] << " ";
        }
        cout << "]" << endl;

        // Shift the array to the left
        int temp = shiftedArray[0];
        for (int i = 0; i < arraySize - 1; i++) {
            shiftedArray[i] = shiftedArray[i + 1];
        }
        shiftedArray[arraySize - 1] = temp;

        if (rank == size - 1) {
            // Send the shifted array to process 0
            MPI_Send(shiftedArray, arraySize, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        else {
            // Send the shifted array to the next process
            MPI_Send(shiftedArray, arraySize, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        }
    }

    double deltaTime = MPI_Wtime() - start;
    if (rank == 0) {
        cout << "Time delta: " << deltaTime;
    }

    MPI_Finalize();

    return 0;
}