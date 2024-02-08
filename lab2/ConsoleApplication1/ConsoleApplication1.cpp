#include <iostream>
#include <vector>
#include <random>
#include "mpi.h"

using namespace std;
using u32 = uint_least32_t;
using engine = std::mt19937;

int getRandNumb(int min, int max) {
	random_device os_seed;
	const u32 seed = os_seed();
	engine generator(seed);
	uniform_int_distribution<u32> distribute(min, max);

	return distribute(generator);
}

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Status status;

	if (size < 2) {
		cout << "This program requires at least 2 processes to run." << endl;
		MPI_Finalize();
		return 1;
	}

	vector<int> packets_received;
	
	// double start = MPI_Wtime();
	int data = getRandNumb(100, 999);

	if (rank == 0) {
		while (packets_received.size() < size - 1) {
			int packet[1];

			// Принимаем пакеты от других процессов
			MPI_Recv(packet, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			// Переадресуем пакет адресату
			MPI_Send(packet, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
			cout << "MASTER " << rank << " redirected packet from process " << status.MPI_SOURCE << " to receiver." << endl;

			// Информируем процесс-источник об успешной доставке
			MPI_Send(nullptr, 0, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);

			packets_received.push_back(status.MPI_SOURCE);
		}
	}
	else {
		// Процессы-источники генерируют и отправляют пакеты процессу 0
		srand(rank); // Для генерации случайных пакетов на разных процессах

		int packet[1];
		packet[0] = data;

		// Генерируем случайный адресат пакета (от 1 до size-1)
		int recipient = 1 + rand() % (size - 1);

		// Отправляем пакет процессу 0
		MPI_Send(packet, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		cout << "Process " << rank << " sent packet to MASTER 0, receiver is: " << recipient << ". Sent data: " << packet[0] << endl;

		// Получаем подтверждение доставки от процесса 0
		MPI_Recv(nullptr, 0, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		cout << "Process " << rank << " received delivery approval." << " Received data: " << packet[0] << endl;
	}

	// double deltaTime = MPI_Wtime() - start;
	// if (rank == 0) {
	//	cout << "Time delta: " << deltaTime;
	// }

	MPI_Finalize();
	
	return 0;
}