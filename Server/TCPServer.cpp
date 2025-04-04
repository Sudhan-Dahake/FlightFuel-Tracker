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
};


TCPServer::~TCPServer() {
	closesocket(this->ServerSocket);

	delete this->previousData;
	this->previousData = nullptr;

	delete this->flightConsumptions;
	this->flightConsumptions = nullptr;

	WSACleanup();
}


void TCPServer::Start() {
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


void TCPServer::HandleClient(SOCKET clientSocket) {
	int recvSize = -1;

	bool isClientDisconnected = false;

	char* RxBuffer = new char[PACKETSIZE];

	memset(RxBuffer, 0, PACKETSIZE);

	while (!isClientDisconnected) {
		recvSize = recv(clientSocket, RxBuffer, PACKETSIZE, 0);

		//std::cout << "Packet Received :)" << std::endl;

		if (recvSize == 0) {
			std::cout << "Client disconnected.\n";

			break;
		}

		if (recvSize < 0) {
			std::cout << "Receive error.\n";
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

		{
			std::lock_guard<std::mutex> lock(data_mutex);

			auto it = this->previousData->find(flightID);

			if (it != this->previousData->end()) {
				int prevTime = it->second.time;

				float prevFuel = it->second.fuel;

				consumption = this->ComputeFuelConsumption(prevTime, prevFuel, currTime, currFuel);

				if (consumption > 0.0f) {
					(*this->flightConsumptions)[flightID].push_back(consumption);
				}
			}
			(*this->previousData)[flightID] = { currTime, currFuel };
		}
	}

	else if (pkt.GetHeader().finishedFlag == 'D') {
		const std::vector<float>& fuelConsumptionRates = (*this->flightConsumptions)[pkt.GetHeader().flightID];

		if (!fuelConsumptionRates.empty()) {
			float sum = 0.0f;
			for (float rate : fuelConsumptionRates) {
				sum += rate;
			}

			float avg = sum / fuelConsumptionRates.size();
			std::string filename = "Avg_Fuel_Consumption_In_Flights.txt";
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
			}
		}
		else {
			std::cout << "Flight ID " << pkt.GetHeader().flightID << " completed, but not enough data to compute average.\n";
		}
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