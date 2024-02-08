// parallel_lab.cpp : Defines the entry point for the console application.

#include <iostream>
#include <math.h>
#include "mpi.h"

// выделение памяти под матрицу
int allocMatrix(int*** mat, int rows, int cols) {
	// Allocate rows*cols contiguous items
	int* p = (int*)malloc(sizeof(int*) * rows * cols);
	if (!p) {
		return -1;
	}
	// Allocate row pointers
	*mat = (int**)malloc(rows * sizeof(int*));
	if (!mat) {
		free(p);
		return -1;
	}

	// Set up the pointers into the contiguous memory
	for (int i = 0; i < rows; i++) {
		(*mat)[i] = &(p[i * cols]);
	}
	return 0;
}
// освобождение памяти от матрицы
int freeMatrix(int*** mat) {
	free(&((*mat)[0][0]));
	free(*mat);
	return 0;
}
// умножение матрицы
void matrixMultiply(int** a, int** b, int rows, int cols, int*** c) {
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			int val = 0;
			for (int k = 0; k < rows; k++) {
				val += a[i][k] * b[k][j];
			}
			(*c)[i][j] = val;
		}
	}
}
// вывод матрицы
void printMatrix(int** mat, int size) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			printf("%d ", mat[i][j]);
		}
		printf("\n");
	}
}
// запись матрицы в файл
void printMatrixFile(int** mat, int size, FILE* fp) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			fprintf(fp, "%d ", mat[i][j]);
		}
		fprintf(fp, "\n");
	}
}

int main(int argc, char* argv[]) {
	MPI_Comm cartComm;
	int dim[2], period[2], reorder;
	int coord[2];
	FILE* fp;
	int** A = NULL, ** B = NULL, ** C = NULL;
	int** localA = NULL, ** localB = NULL, ** localC = NULL;
	int** localARec = NULL, ** localBRec = NULL;
	int rows = 0;
	int columns;
	int count = 0;
	int rank;
	int worldSize;
	int procDim;
	int blockDim;
	int left, right, up, down;
	int bCastData[4];

	double start, end; // для измерения времени

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	start = MPI_Wtime();

	if (rank == 0) {
		int n;
		char ch;

		// определение размерности матрицы
		fp = fopen("A.txt", "r"); // открытие .txt-файла с матрицей на чтение
		if (fp == NULL) {
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		while (fscanf(fp, "%d", &n) != EOF) { // подсчёт числа строк
			ch = fgetc(fp);
			if (ch == '\n') {
				rows = rows + 1;
			}
			count++;
		}
		columns = count / rows; // столбцы = общее число элементов / строки

		// проверка матрицы и числа процессов
		if (columns != rows) { // проверка на квадратную матрицу
			printf("[ERROR] Matrix must be square!\n");
			MPI_Abort(MPI_COMM_WORLD, 2);
		}
		double sqroot = sqrt(worldSize); // проверка, является ли число процессов квадратом целого числа
		if ((sqroot - floor(sqroot)) != 0) {
			printf("[ERROR] Number of processes must be a perfect square!\n");
			MPI_Abort(MPI_COMM_WORLD, 2);
		}
		int intRoot = (int)sqroot; // проверка на делимость строк и столбцов матрицы на корень числа процессов
		if (columns % intRoot != 0 || rows % intRoot != 0) {
			printf("[ERROR] Number of rows/columns not divisible by %d!\n", intRoot);
			MPI_Abort(MPI_COMM_WORLD, 3);
		}
		procDim = intRoot; // размерность сети процессов
		blockDim = columns / intRoot; // размер блока

		fseek(fp, 0, SEEK_SET); // переустановка указателя на файл в начало, чтобы повторно прочитать содержимое

		// выделение памяти под матрицы А и В
		if (allocMatrix(&A, rows, columns) != 0) {
			printf("[ERROR] Matrix alloc for A failed!\n");
			MPI_Abort(MPI_COMM_WORLD, 4);
		}
		if (allocMatrix(&B, rows, columns) != 0) {
			printf("[ERROR] Matrix alloc for B failed!\n");
			MPI_Abort(MPI_COMM_WORLD, 5);
		}

		// чтение матрицы А и заполнение ячеек
		for (int i = 0; i < rows; i++) { 
			for (int j = 0; j < columns; j++) {
				fscanf(fp, "%d", &n);
				A[i][j] = n;
			}
		}
		// printf("A matrix:\n"); // вывод в консоль матрицы А
		// printMatrix(A, rows);
		fclose(fp); // закрытие файла

		// чтение матрицы В и заполнение ячеек
		fp = fopen("B.txt", "r");
		if (fp == NULL) {
			return 1;
		}
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < columns; j++) {
				fscanf(fp, "%d", &n);
				B[i][j] = n;
			}
		}
		// printf("B matrix:\n"); // вывод в консоль матрицы В
		// printMatrix(B, rows);
		fclose(fp); // закрытие файла

		// выделение памяти под матрицу С
		if (allocMatrix(&C, rows, columns) != 0) {
			printf("[ERROR] Matrix alloc for C failed!\n");
			MPI_Abort(MPI_COMM_WORLD, 6);
		}

		// заполняем массив bCastData
		bCastData[0] = procDim; // размерность сети процессов
		bCastData[1] = blockDim; // размер блока
		bCastData[2] = rows; // строки
		bCastData[3] = columns; // столбцы
	}


	// создаём двумерную решетку процессов
	MPI_Bcast(&bCastData, 4, MPI_INT, 0, MPI_COMM_WORLD);
	procDim = bCastData[0];
	blockDim = bCastData[1];
	rows = bCastData[2];
	columns = bCastData[3];

	dim[0] = procDim; dim[1] = procDim;
	period[0] = 1; period[1] = 1;
	reorder = 1;
	MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &cartComm);

	// выделяем памяти под локальные блоки матриц А и В
	allocMatrix(&localA, blockDim, blockDim);
	allocMatrix(&localB, blockDim, blockDim);

	// создаём тип данных для описания подмассивов в глобальном массиве
	int globalSize[2] = { rows, columns }; // массив с размерами всей матрицы
	int localSize[2] = { blockDim, blockDim }; // массив с размерами блока
	int starts[2] = { 0,0 }; // начальные индексы блока
	MPI_Datatype type, subarrtype;
	MPI_Type_create_subarray(2, globalSize, localSize, starts, MPI_ORDER_C, MPI_INT, &type); // тип данных блока матрицы
	MPI_Type_create_resized(type, 0, blockDim * sizeof(int), &subarrtype); // тип данных чтобы учесть смещение блока
	MPI_Type_commit(&subarrtype); // фиксируем созданный тип данных

	// создаем указатели для доступа к глобальным матрицам (только для процесса с рангом 0)
	int* globalptrA = NULL;
	int* globalptrB = NULL;
	int* globalptrC = NULL;
	if (rank == 0) {
		globalptrA = &(A[0][0]);
		globalptrB = &(B[0][0]);
		globalptrC = &(C[0][0]);
	}

	// распределение данных глобальных матриц A и B по локальным блокам на каждом процессе
	// выделение памяти для массивов 
	int* sendCounts = (int*)malloc(sizeof(int) * worldSize); // число элементов, отправляемых каждому процессу
	int* displacements = (int*)malloc(sizeof(int) * worldSize); // смещение данных, начиная с которого отправляются данные каждому процессу

	if (rank == 0) {
		for (int i = 0; i < worldSize; i++) {
			sendCounts[i] = 1; // sendCounts = 1 для каждого процесса
		}
		int disp = 0;
		for (int i = 0; i < procDim; i++) {
			for (int j = 0; j < procDim; j++) {
				displacements[i * procDim + j] = disp;
				disp += 1;
			}
			disp += (blockDim - 1) * procDim;
		}
	}

	// рассылаем данные матриц A и B по локальным блокам localA и localB каждого процесса
	MPI_Scatterv(globalptrA, sendCounts, displacements, subarrtype, &(localA[0][0]),
		rows * columns / (worldSize), MPI_INT,
		0, MPI_COMM_WORLD);
	MPI_Scatterv(globalptrB, sendCounts, displacements, subarrtype, &(localB[0][0]),
		rows * columns / (worldSize), MPI_INT,
		0, MPI_COMM_WORLD);

	// выделение памяти для локальной матрицы C
	if (allocMatrix(&localC, blockDim, blockDim) != 0) {
		printf("[ERROR] Matrix alloc for localC in rank %d failed!\n", rank);
		MPI_Abort(MPI_COMM_WORLD, 7);
	}

	// изначальный сдвиг
	MPI_Cart_coords(cartComm, rank, 2, coord); // получение координат текущего процесса в массив coords
	MPI_Cart_shift(cartComm, 1, coord[0], &left, &right); // вычисление соседей по оси Х (слева и справа)
	MPI_Sendrecv_replace(&(localA[0][0]), blockDim * blockDim, MPI_INT, left, 1, right, 1, cartComm, MPI_STATUS_IGNORE); // обмен данными между процессами вдоль оси x
	MPI_Cart_shift(cartComm, 0, coord[1], &up, &down); // вычисление соседей по оси Y (сверху и снизу)
	MPI_Sendrecv_replace(&(localB[0][0]), blockDim * blockDim, MPI_INT, up, 1, down, 1, cartComm, MPI_STATUS_IGNORE); // обмен данными между процессами вдоль оси y:

	// инициализация блока в матрице С
	for (int i = 0; i < blockDim; i++) {
		for (int j = 0; j < blockDim; j++) {
			localC[i][j] = 0;
		}
	}

	// выделение памяти для временной матрицы multiplyRes:
	int** multiplyRes = NULL; // multiplyRes - временное хранение промежуточных результатов умножения
	if (allocMatrix(&multiplyRes, blockDim, blockDim) != 0) {
		printf("[ERROR] Matrix alloc for multiplyRes in rank %d failed!\n", rank);
		MPI_Abort(MPI_COMM_WORLD, 8);
	}

	// перемножение блоков localA и localB и обновление localC
	for (int k = 0; k < procDim; k++) {
		matrixMultiply(localA, localB, blockDim, blockDim, &multiplyRes);

		for (int i = 0; i < blockDim; i++) {
			for (int j = 0; j < blockDim; j++) {
				localC[i][j] += multiplyRes[i][j];
			}
		}

		// 1 раз: сдвиг блока матрицы А (влево) и блока матрицы В (вверх)
		MPI_Cart_shift(cartComm, 1, 1, &left, &right);
		MPI_Cart_shift(cartComm, 0, 1, &up, &down);
		MPI_Sendrecv_replace(&(localA[0][0]), blockDim * blockDim, MPI_INT, left, 1, right, 1, cartComm, MPI_STATUS_IGNORE);
		MPI_Sendrecv_replace(&(localB[0][0]), blockDim * blockDim, MPI_INT, up, 1, down, 1, cartComm, MPI_STATUS_IGNORE);
	}

	// сбор результатов
	MPI_Gatherv(&(localC[0][0]), rows * columns / worldSize, MPI_INT,
		globalptrC, sendCounts, displacements, subarrtype,
		0, MPI_COMM_WORLD);

	// освобождение памяти локальных матриц
	freeMatrix(&localC);
	freeMatrix(&multiplyRes);

	// работа с результатами
	if (rank == 0) {
		// вывести результаты в консоли
		// printf("C is:\n");
		// printMatrix(C, rows);

		// сохранить результаты в файл output.txt
		FILE* output_file = fopen("output.txt", "w");
		if (output_file == NULL) {
			fprintf(stderr, "Error opening output file.\n");
			MPI_Abort(MPI_COMM_WORLD, 9);
		}
		printMatrixFile(C, rows, output_file);
		fclose(output_file);
		
		// вывод времени выполнения в консоль
		end = MPI_Wtime();
		printf("Time: %.4fs\n", end - start); // точность времени: до 4 чисел после запятой
	}

	MPI_Finalize();

	return 0;
}