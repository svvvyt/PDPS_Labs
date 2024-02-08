#include <iostream>
#include "mpi.h"

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int total_procs, proc_rank;

    MPI_Comm_size(MPI_COMM_WORLD, &total_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

    srand(time(NULL) + proc_rank); // генератор случайных чисел для каждого процесса с использованием его ранга

    int secret_number = rand() % 9 + 1; // случайное число от 1 до 9 для каждого процесса

    int grid_dim_1 = 2, grid_dim_2 = 2, grid_dim_3 = total_procs / 4; // размеры трехмерной решетки процессов
    int grid_dims[3] = { grid_dim_1, grid_dim_2, grid_dim_3 }; // массив с этими размерами
    int grid_periods[3] = { 0, 0, 0 }; // будут ли циклические периодические условия в каждом измерении

    // коммуникатор для трехмерной решетки
    MPI_Comm three_dim_grid_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 3, grid_dims, grid_periods, 0, &three_dim_grid_comm);

    // координаты текущего процесса в трехмерной решетке
    int grid_coords[3];
    MPI_Cart_coords(three_dim_grid_comm, proc_rank, 3, grid_coords);
    printf("Process %2d. 3D coordinates: (%d %d %d). Secret number: %d\n", proc_rank, grid_coords[0], grid_coords[1], grid_coords[2], secret_number);

    // коммуникатор для столбца в трехмерной решетке
    int column_subdims[3] = { 0, 0, 1 };
    MPI_Comm column_comm;
    MPI_Cart_sub(three_dim_grid_comm, column_subdims, &column_comm);

    // ранг текущего процесса в новом коммуникаторе
    int column_rank;
    MPI_Comm_rank(column_comm, &column_rank);
    printf("Old rank: %d, New rank in column communicator: %d\n", proc_rank, column_rank);

    // умножение всех сгенерированных чисел в каждом столбце трехмерной решетки
    int in[1] = { secret_number }, out[1];
    MPI_Reduce(in, out, 1, MPI_INT, MPI_PROD, 0, column_comm);

    // результат умножения только для процесса с рангом 0 в новом коммуникаторе
    if (column_rank == 0) {
        printf("*** Process %2d result after multiplication in the column: %d ***\n", proc_rank, out[0]);
    }

    MPI_Finalize();

    return 0;
}