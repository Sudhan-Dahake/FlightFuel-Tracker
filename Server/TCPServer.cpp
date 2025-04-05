#include "TCPServer.h"

TCPServer::TCPServer(int port, int threadPoolSize)
	: threadPool(threadPoolSize) {

	//starts Winsock DLLs		
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw std::runtime_error("Failed to initialize Winsock.");
	};


	//create server socket
	this->ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->ServerSocket == INVALID_SOCKET) {
		WSACleanup();

		throw std::runtime_error("Socket Creation Failed.");
	};


	//binds socket to address
	this->SvrAddr.sin_family = AF_INET;
	this->SvrAddr.sin_addr.s_addr = INADDR_ANY;
	this->SvrAddr.sin_port = htons(port);

	if (bind(ServerSocket, (struct sockaddr*)&(this->SvrAddr), sizeof(this->SvrAddr)) == SOCKET_ERROR)
	{
		closesocket(this->ServerSocket);

		WSACleanup();

		throw std::runtime_error("Binding Failed.");
	};


	if (listen(this->ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
		closesocket(this->ServerSocket);

		WSACleanup();

		throw std::runtime_error("Listening Failed.");
	}

	/*std::cout << "UDP Server listening on port: " << port << "..." << "\n";*/

	this->port = port;

	this->previousData = new std::unordered_map<int, FlightSnapshot>();

	this->flightConsumptions = new std::unordered_map<int, std::vector<float>>();

	this->serverRunning.store(true);
};


TCPServer::~TCPServer() {
	this->serverRunning.store(false);

	if (this->flusherThread.joinable()) {
		this->flusherThread.join();
	};


	closesocket(this->ServerSocket);

	delete this->previousData;
	this->previousData = nullptr;

	delete this->flightConsumptions;
	this->flightConsumptions = nullptr;

	WSACleanup();
}


void TCPServer::Start() {
	// Starting the background flusher thread for file I/O.
	this->flusherThread = std::thread(&TCPServer::BackgroundFlusherForFile, this);

	while (true) {
		std::cout << "TCP Server ready\n";

		SOCKET clientSocket = accept(this->ServerSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET) {
			throw std::runtime_error("Failed to accept client connections.");

			continue;
		};

		std::cout << "Client connected\n";

		// Offload to thread pool
		boost::asio::post(threadPool, [this, clientSocket]() {
			this->HandleClient(clientSocket);
		});
	};
};


// *************** OPTIMIZATION 2 STARTS HERE ****************

void TCPServer::BackgroundFlusherForFile() {
	const int FLUSH_THRESHOLD = 20;

	const std::chrono::seconds FLUSH_INTERVAL(2);

	auto lastFlushTime = std::chrono::steady_clock::now();

	while (this->serverRunning.load()) {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		std::vector<std::pair<int, float>> toFlush;

		auto now = std::chrono::steady_clock::now();

		{
			std::lock_guard<std::mutex> lock(this->avgBufferForFileWritingMutex);

			if ((this->avgBufferForFileWriting.size() >= FLUSH_THRESHOLD) || ((now - lastFlushTime) >= FLUSH_INTERVAL)) {
				toFlush.swap(this->avgBufferForFileWriting);
			};
		}


		if (!toFlush.empty()) {
			std::string filename = "Avg_Fuel_Consumption_In_Flights.txt";

			std::ofstream outFile;

			std::ifstream checkFile(filename);

			bool fileExists = checkFile.good();

			checkFile.close();

			outFile.open(filename, std::ios::app);

			if (!fileExists) {
				outFile << "FlightId\tAverage Fuel Consumption\n";
			};

			for (const auto& [flightID, avg] : toFlush) {
				outFile << flightID << "\t" << avg << "\n";
			};


			outFile.close();
		};
	};


	// Final Flush on shutdown.
	if (!this->avgBufferForFileWriting.empty()) {
		std::lock_guard<std::mutex> lock(avgBufferForFileWritingMutex);

		std::ofstream outFile("Avg_Fuel_Consumption_In_Flights.txt", std::ios::app);

		for (const auto& [flightID, avg] : this->avgBufferForFileWriting) {
			outFile << "FlightId\tAverage Fuel Consumption\n";
		};

		this->avgBufferForFileWriting.clear();
	};
};


// *************** OPTIMIZATION 2 ENDS HERE ****************



void TCPServer::HandleClient(SOCKET clientSocket) {
	int recvSize = -1;

	bool isClientDisconnected = false;

	char* RxBuffer = new char[PACKETSIZE];

	memset(RxBuffer, 0, PACKETSIZE);

	while (!isClientDisconnected) {
		recvSize = recv(clientSocket, RxBuffer, PACKETSIZE, 0);

		if (recvSize == 0) {
			std::cout << "Client disconnected.\n";

			break;
		}

		if (recvSize < 0) {
			int err = WSAGetLastError();

			std::cout << "Received error. Code: " << err << std::endl;

			if (err == WSAEWOULDBLOCK || err == WSAEINTR) {
				// Non-fatal. We are going to wait and retry.
				continue;
			}

			// Fatal error — disconnecting this client. This is for other errors.
			break;
		}

		this->HandlePacket(clientSocket, RxBuffer, isClientDisconnected);
	}

	delete[] RxBuffer;
	RxBuffer = nullptr;

	closesocket(clientSocket);
}




void TCPServer::HandlePacket(SOCKET clientSocket, char* RxBuffer, bool& isClientDisconnected) {
	Packet pkt(RxBuffer);

	if (pkt.IsBodyPresent()) {
		Header head = pkt.GetHeader();

		FlightData flightData = pkt.GetFlightData();

		int flightID = head.flightID;

		int currTime = flightData.timeStamp; //(flightData.timeStamp.hour * 3600) + (flightData.timeStamp.minute * 60) + flightData.timeStamp.second;

		float currFuel = flightData.fuelAmount;

		float consumption = 0.0f;

		bool hasPrevData = false;

		int prevTime;

		float prevFuel;

		// ********* OPTIMIZATION 1 STARTS HERE *****************


		// Shared lock is used so that many threads can read at the same time when there is no active writer.
		{
			std::shared_lock<std::shared_mutex> lock(this->previousDataSharedMutex);

			auto it = this->previousData->find(flightID);

			if (it != this->previousData->end()) {
				prevTime = it->second.time;

				prevFuel = it->second.fuel;

				hasPrevData = true;
			}
		}


		if (hasPrevData) {
			consumption = this->ComputeFuelConsumption(prevTime, prevFuel, currTime, currFuel);

			if (consumption > 0.0f) {

				// lock_guard makes sure the lock is freed even if an exception occurs.
				// only one thread can update the FlightConsumptions Data structure.
				std::unique_lock<std::shared_mutex> lock(this->flightConsumptionsSharedMutex);

				(*this->flightConsumptions)[flightID].push_back(consumption);
			}
		}


		// Unique_lock ensures only one writer has the lock and there are no reader when there is a writer.
		{
			std::unique_lock<std::shared_mutex> lock(this->previousDataSharedMutex);

			(*this->previousData)[flightID] = { currTime, currFuel };
		}


		// ********* OPTIMIZATION 1 ENDS HERE *****************
	}

	else if (pkt.IsFinishedFlagSet()) {
		std::vector<float> fuelConsumptionRates;

		{
			std::shared_lock<std::shared_mutex> lock(this->flightConsumptionsSharedMutex);

			std::unordered_map<int, std::vector<float>>::const_iterator it = this->flightConsumptions->find(pkt.GetHeader().flightID);

			if (it != this->flightConsumptions->end()) {
				fuelConsumptionRates = it->second;
			};
		}

		if (!fuelConsumptionRates.empty()) {
			float sum = 0.0f;

			for (float rate : fuelConsumptionRates) {
				sum += rate;
			}

			float avg = sum / fuelConsumptionRates.size();


			// *************** OPTIMIZATION 2 STARTS HERE ****************


			{
				std::lock_guard<std::mutex> lock(this->avgBufferForFileWritingMutex);

				this->avgBufferForFileWriting.emplace_back(pkt.GetHeader().flightID, avg);
			}



			// *************** OPTIMIZATION 2 ENDS HERE *******************

			/*std::string filename = "Avg_Fuel_Consumption_In_Flights.txt";

			std::ofstream outFile;

			{
				std::lock_guard<std::mutex> lock(fileMutex);

				std::ifstream checkFile(filename);

				bool fileExists = checkFile.good();

				checkFile.close();

				outFile.open(filename, std::ios::app);

				if (!fileExists) {
					outFile << "FlightId\tAverage Fuel Consumption\n";
				}

				outFile << pkt.GetHeader().flightID << "\t" << avg << "\n";

				outFile.close();
			}*/
		}

		else {
			std::cout << "Flight ID " << pkt.GetHeader().flightID << " completed, but not enough data to compute average.\n";
		};
		
		std::cout << "Client completed\n";

		isClientDisconnected = true;
	}
};


//int TCPServer::ConvertToSeconds(const TimeInfo& t) {
//	return ((t.hour * 3600) + (t.minute * 60) + t.second);
//};



float TCPServer::ComputeFuelConsumption(const int& prevTime, float prevFuel, const int& currTime, float currFuel) {

	int deltaT = currTime - prevTime;

	float deltaFuel = prevFuel - currFuel;

	if (deltaT <= 0) {
		return 0.0f;
	};

	return (deltaFuel / deltaT);		// This will be in units/second.
};