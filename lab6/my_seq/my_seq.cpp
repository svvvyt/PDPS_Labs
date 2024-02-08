#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std;
using namespace chrono;

// Функция для чтения матрицы из файла
vector<vector<int>> readMatrixFromFile(const string& fileName) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << fileName << endl;
        exit(EXIT_FAILURE);
    }

    vector<vector<int>> matrix;
    int value;

    while (file >> value) {
        matrix.push_back({ value });
        char c;
        while (file.get(c) && c != '\n' && c != '\r') {
            file >> value;
            matrix.back().push_back(value);
        }
    }

    file.close();
    return matrix;
}

// Функция для записи матрицы в файл
void writeMatrixToFile(const string& fileName, const vector<vector<int>>& matrix) {
    ofstream file(fileName);
    if (!file.is_open()) {
        cerr << "Unable to open file: " << fileName << endl;
        exit(EXIT_FAILURE);
    }

    for (const auto& row : matrix) {
        for (int value : row)
            file << value << " ";
        file << endl;
    }

    file.close();
}

// Функция для вывода матрицы в консоль
void printMatrix(const vector<vector<int>>& matrix) {
    for (const auto& row : matrix) {
        for (int value : row)
            cout << value << " ";
        cout << endl;
    }
}

// Функция для перемножения двух матриц
vector<vector<int>> multiplyMatrices(const vector<vector<int>>& matrixA,
    const vector<vector<int>>& matrixB) {
    int size = matrixA.size();
    vector<vector<int>> result(size, vector<int>(size, 0));

    for (int i = 0; i < size; ++i)
        for (int j = 0; j < size; ++j)
            for (int k = 0; k < size; ++k)
                result[i][j] += matrixA[i][k] * matrixB[k][j];

    return result;
}

int main() {
    // Чтение матриц из файлов
    vector<vector<int>> matrixA = readMatrixFromFile("A.txt");
    vector<vector<int>> matrixB = readMatrixFromFile("B.txt");

    // Начало отсчета времени
    auto start = high_resolution_clock::now();

    // Перемножение матриц
    vector<vector<int>> resultMatrix = multiplyMatrices(matrixA, matrixB);

    // Запись результата в файл
    writeMatrixToFile("C.txt", resultMatrix);

    // Вывод результата в консоль
    // cout << "Matrix A:" << endl;
    // printMatrix(matrixA);

    // cout << "\nMatrix B:" << endl;
    // printMatrix(matrixB);

    // cout << "\nResult Matrix C:" << endl;
    // printMatrix(resultMatrix);

    // Конец отсчета времени и расчёт длительности
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    // Вывод времени выполнения
    cout << "Time taken by function: " << duration.count() / 1000000.0 << " seconds" << endl;

    return 0;
}