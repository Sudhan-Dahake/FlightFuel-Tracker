#include "UDPServer.h"

UDPServer::UDPServer(int port, int threadPoolSize)
	: FlightIdCounter(1), threadPool(threadPoolSize) {
	
	//starts Winsock DLLs		
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		throw std::runtime_error("Failed to initialize Winsock.");
	};


	//create server socket
	this->ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

	std::cout << "UDP Server listening on port: " << port << "..." << "\n";
};

UDPServer::~UDPServer() {
	closesocket(this->ServerSocket);

	WSACleanup();
};


void UDPServer::Start() {
	char RxBuffer[PACKETSIZE] = {};

	sockaddr_in CltAddr;

	int len = sizeof(struct sockaddr_in);

	while (true) {
		int recvSize = recvfrom(this->ServerSocket, RxBuffer, sizeof(RxBuffer), 0, (struct sockaddr*)&CltAddr, &len);

		std::cout << "Receive Size: " << recvSize << std::endl;

		bool IsPacketValid = true;

		if (recvSize < 0) {
			IsPacketValid = false;

			closesocket(ServerSocket);

			WSACleanup();

			throw std::runtime_error("Receiving failed.");
		};

		// Copy data to new buffer for thread safety
		char* bufferCopy = new char[PACKETSIZE];

		memcpy(bufferCopy, RxBuffer, PACKETSIZE);

		sockaddr_in clientAddrCopy = CltAddr;

		// Offload to thread pool
		boost::asio::post(threadPool, [this, bufferCopy, clientAddrCopy, IsPacketValid]() {
			this->HandlePacket(bufferCopy, clientAddrCopy, IsPacketValid);

			delete[] bufferCopy;  // Clean up after processing
		});
	};
};


void UDPServer::HandlePacket(char* RxBuffer, sockaddr_in CltAddr, bool IsPacketValid) {
	int len = sizeof(sockaddr_in);

	Packet pkt(RxBuffer);
	
	/*Header header = IsPacketValid ? confirmationPkt.SendConfirmation(pkt.GetFlightId(), 'P') : confirmationPkt.SendConfirmation(pkt.GetFlightId(), 'F');*/

	char TxBuffer[PACKETSIZE] = {};

	/*sendMutex.lock();

	memcpy(TxBuffer, &(this->FlightIdCounter), sizeof(int));
	
	int sendSize = sendto(this->ServerSocket, TxBuffer, sizeof(int), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));
	
	std::cout << "First SendTo in UDPServer: " << sendSize << std::endl;

	this->FlightIdCounter.fetch_add(1);

	sendMutex.unlock();*/

	if (pkt.IsBodyPresent()) {
		Packet confirmationPkt;

		Header header = IsPacketValid ? confirmationPkt.SendConfirmation(pkt.GetFlightId(), 'P') : confirmationPkt.SendConfirmation(pkt.GetFlightId(), 'F');

		memcpy(TxBuffer, &header, sizeof(Header));

		int sendConfirmationSize = sendto(this->ServerSocket, TxBuffer, sizeof(Header), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));

		std::cout << "First Confirmation in UDPServer: " << sendConfirmationSize << std::endl;

		Header head = pkt.GetHeader();

		FlightData flightData = pkt.GetFlightData();

		int flightID = head.flightID;

		TimeInfo currTime = flightData.timeStamp;

		float currFuel = flightData.fuelAmount;

		float consumption = 0.0f;

		{
			std::lock_guard<std::mutex> lock(data_mutex);

			auto it = previousData.find(flightID);

			if (it != previousData.end()) {
				TimeInfo prevTime = it->second.time;

				float prevFuel = it->second.fuel;

				consumption = this->ComputeFuelConsumption(prevTime, prevFuel, currTime, currFuel);

				if (consumption > 0.0f) {
					this->flightConsumptions[flightID].push_back(consumption);
				};
			};

			previousData[flightID] = { currTime, currFuel };
		}
	}

	else if (pkt.IsFinishedFlagSet()) {
		const std::vector<float>& fuelConsumptionRates = this->flightConsumptions[pkt.GetHeader().flightID];

		if (!fuelConsumptionRates.empty()) {
			float sum = 0.0f;

			for (float rate : fuelConsumptionRates) {
				sum += rate;
			}

			float avg = sum / fuelConsumptionRates.size();

			std::cout << "Flight ID: " << pkt.GetHeader().flightID << ", Average Fuel Consumption: " << avg << "\n";
		}

		else {
			std::cout << "Flight ID " << pkt.GetHeader().flightID << " completed, but not enough data to compute average.\n";
		};
	}

	else {
		sendMutex.lock();

		memcpy(TxBuffer, &(this->FlightIdCounter), sizeof(int));

		int sendSize = sendto(this->ServerSocket, TxBuffer, sizeof(int), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));

		std::cout << "First SendTo in UDPServer: " << sendSize << std::endl;

		this->FlightIdCounter.fetch_add(1);

		sendMutex.unlock();
	};
};



//void UDPServer::Start() {
//	char RxBuffer[PACKETSIZE] = {};
//
//	sockaddr_in CltAddr;					//Client Address for sending responses
//	int len = sizeof(struct sockaddr_in);	//Length parameter for the recvfrom function call
//
//	int recvSize = recvfrom(this->ServerSocket, RxBuffer, sizeof(RxBuffer), 0, (struct sockaddr*)&CltAddr, &len);
//	// using recvfrom because we are using UDP protocol
//	// It takes six arguments, socket to use, Data to be received, number of bytes, flags, pointer to address, and size of address.
//
//	if (recvSize < 0) {
//		// if recvSize fails then this if block gets executed.
//
//		closesocket(ServerSocket);
//		// Closing the socket.
//
//		WSACleanup();
//		// Cleaning up
//
//		throw std::runtime_error("Initial Receiving Failed.");
//		// exiting the application.
//	};
//
//	char TxBuffer[PACKETSIZE] = {};
//
//	memcpy(TxBuffer, &(this->FlightIdCounter), sizeof(int));
//
//	sendto(this->ServerSocket, TxBuffer, sizeof(int), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));
//
//	this->FlightIdCounter = this->FlightIdCounter.fetch_add(1);
//
//	bool endOfDataFromClient = false;
//
//	while (!endOfDataFromClient) {
//		recvSize = recvfrom(this->ServerSocket, RxBuffer, sizeof(RxBuffer), 0, (struct sockaddr*)&CltAddr, &len);
//
//		if (recvSize < 0) {
//			// if recvSize fails then this if block gets executed.
//
//			closesocket(ServerSocket);
//			// Closing the socket.
//
//			WSACleanup();
//			// Cleaning up
//
//			throw std::runtime_error("Initial Receiving Failed.");
//			// exiting the application.
//		};
//
//		Packet pkt(RxBuffer);		// Deserializing the packet.
//
//		Packet confirmationPkt;
//
//		Header header = confirmationPkt.SendConfirmation(pkt.GetFlightId());
//
//		memcpy(TxBuffer, &header, sizeof(Header));
//
//		// Sends a confirmation packet.
//		// The packet only contains the header.
//		sendto(this->ServerSocket, TxBuffer, sizeof(Header), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));
//
//		if (pkt.GetHeader().finishedFlag == 'D') {
//			endOfDataFromClient = true;
//
//			const std::vector<float>& fuelConsumptionRates = this->flightConsumptions[pkt.GetHeader().flightID];
//
//			if (!fuelConsumptionRates.empty()) {
//				float sum = 0.0f;
//
//				for (float rate : fuelConsumptionRates) {
//					sum += rate;
//				};
//
//				float averageFuelConsumption = sum / fuelConsumptionRates.size();
//
//				std::cout << "Flight ID: " << pkt.GetHeader().flightID << ", average Fuel Consumption: " << averageFuelConsumption << "\n";
//			}
//
//			else {
//				std::cout << "Flight ID " << pkt.GetHeader().flightID << " completed, but not enough data to compute average consumption.\n";
//			};
//		}
//
//		else {
//			Header head = pkt.GetHeader();
//
//			FlightData flightData = pkt.GetFlightData();
//
//			int flightID = head.flightID;
//
//			TimeInfo currTime = flightData.timeStamp;
//
//			float currFuel = flightData.fuelAmount;
//
//			float consumption = 0.0f;
//
//			{
//				std::lock_guard<std::mutex> lock(data_mutex);
//
//				auto it = previousData.find(flightID);
//				if (it != previousData.end()) {
//					// We have previous data, calculate consumption
//					TimeInfo prevTime = it->second.time;
//
//					float prevFuel = it->second.fuel;
//
//					consumption = this->ComputeFuelConsumption(prevTime, prevFuel, currTime, currFuel);
//
//					if (consumption > 0.0f) {
//						this->flightConsumptions[flightID].push_back(consumption);
//					};
//				}
//
//				// Updating the snapshot for next time.
//				previousData[flightID] = { currTime, currFuel };
//			}
//		}
//	};
//}

int UDPServer::ConvertToSeconds(const TimeInfo& t) {
	return ((t.hour * 3600) + (t.minute * 60) + t.second);
};

float UDPServer::ComputeFuelConsumption(const TimeInfo& prevTime, float prevFuel, const TimeInfo& currTime, float currFuel) {
	int deltaT = this->ConvertToSeconds(currTime) - this->ConvertToSeconds(prevTime);

	float deltaFuel = prevFuel - currFuel;

	if (deltaT <= 0) {
		return 0.0f;
	};

	return (deltaFuel / deltaT);		// This will be in units/second.
};

std::string GetClientKey(const sockaddr_in& addr) {
	/*return std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(ntohs(addr.sin_port));*/

	return "Hello World";
};