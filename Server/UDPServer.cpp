//#include "UDPServer.h"
//
//UDPServer::UDPServer(int port, int threadPoolSize)
//	: FlightIdCounter(1), threadPool(threadPoolSize) {
//	
//	//starts Winsock DLLs		
//	WSADATA wsaData;
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//		throw std::runtime_error("Failed to initialize Winsock.");
//	};
//
//
//	//create server socket
//	this->ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//	if (this->ServerSocket == INVALID_SOCKET) {
//		WSACleanup();
//
//		throw std::runtime_error("Socket Creation Failed.");
//	};
//
//
//	//binds socket to address
//	this->SvrAddr.sin_family = AF_INET;
//	this->SvrAddr.sin_addr.s_addr = INADDR_ANY;
//	this->SvrAddr.sin_port = htons(port);
//	if (bind(ServerSocket, (struct sockaddr*)&(this->SvrAddr), sizeof(this->SvrAddr)) == SOCKET_ERROR)
//	{
//		closesocket(this->ServerSocket);
//
//		WSACleanup();
//
//		throw std::runtime_error("Binding Failed.");
//	};
//
//	/*std::cout << "UDP Server listening on port: " << port << "..." << "\n";*/
//
//	this->port = port;
//};
//
//UDPServer::~UDPServer() {
//	closesocket(this->ServerSocket);
//
//	WSACleanup();
//};
//
//
//void UDPServer::Start() {
//	char RxBuffer[PACKETSIZE] = {};
//
//	sockaddr_in CltAddr;
//
//	int len = sizeof(struct sockaddr_in);
//
//	while (true) {
//		std::cout << "UDP Server listening on port: " << this->port << "...\n";
//
//		int recvSize = recvfrom(this->ServerSocket, RxBuffer, sizeof(RxBuffer), 0, (struct sockaddr*)&CltAddr, &len);
//
//		std::cout << "Packet received...\n";
//
//		bool IsPacketValid = true;
//
//		if (recvSize < 0) {
//			IsPacketValid = false;
//
//			closesocket(ServerSocket);
//
//			WSACleanup();
//
//			throw std::runtime_error("Receiving failed.");
//		};
//
//		// Copy data to new buffer for thread safety
//		char* bufferCopy = new char[PACKETSIZE];
//
//		memcpy(bufferCopy, RxBuffer, PACKETSIZE);
//
//		sockaddr_in clientAddrCopy = CltAddr;
//
//		// Offload to thread pool
//		boost::asio::post(threadPool, [this, bufferCopy, clientAddrCopy, IsPacketValid]() {
//			this->HandlePacket(bufferCopy, clientAddrCopy, IsPacketValid);
//
//			delete[] bufferCopy;  // Clean up after processing
//		});
//	};
//};
//
//
//void UDPServer::HandlePacket(char* RxBuffer, sockaddr_in CltAddr, bool IsPacketValid) {
//	int len = sizeof(sockaddr_in);
//
//	Packet pkt(RxBuffer);
//
//	char TxBuffer[PACKETSIZE] = {};
//
//	if (pkt.IsBodyPresent()) {
//		Packet confirmationPkt;
//
//		Header header = IsPacketValid ? confirmationPkt.SendConfirmation(pkt.GetFlightId(), 'P') : confirmationPkt.SendConfirmation(pkt.GetFlightId(), 'F');
//
//		memcpy(TxBuffer, &header, sizeof(Header));
//
//		int sendConfirmationSize = sendto(this->ServerSocket, TxBuffer, sizeof(Header), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));
//
//		std::cout << "Confirmation Sent..." << sendConfirmationSize << std::endl;
//
//		Header head = pkt.GetHeader();
//
//		FlightData flightData = pkt.GetFlightData();
//
//		int flightID = head.flightID;
//
//		TimeInfo currTime = flightData.timeStamp;
//
//		float currFuel = flightData.fuelAmount;
//
//		float consumption = 0.0f;
//
//		{
//			std::lock_guard<std::mutex> lock(data_mutex);
//
//			auto it = previousData.find(flightID);
//
//			if (it != previousData.end()) {
//				TimeInfo prevTime = it->second.time;
//
//				float prevFuel = it->second.fuel;
//
//				consumption = this->ComputeFuelConsumption(prevTime, prevFuel, currTime, currFuel);
//
//				if (consumption > 0.0f) {
//					this->flightConsumptions[flightID].push_back(consumption);
//				};
//			};
//
//			previousData[flightID] = { currTime, currFuel };
//		}
//	}
//
//	else if (pkt.IsFinishedFlagSet()) {
//		const std::vector<float>& fuelConsumptionRates = this->flightConsumptions[pkt.GetHeader().flightID];
//
//		if (!fuelConsumptionRates.empty()) {
//			float sum = 0.0f;
//
//			for (float rate : fuelConsumptionRates) {
//				sum += rate;
//			}
//
//			float avg = sum / fuelConsumptionRates.size();
//
//			std::string filename = "Avg_Fuel_Consumption_In_Flights.txt";
//			std::ofstream outFile;
//
//			{
//				std::lock_guard<std::mutex> lock(fileMutex);
//
//				// Check if file exists
//				std::ifstream checkFile(filename);
//				bool fileExists = checkFile.good();
//				checkFile.close();
//
//				// Open file in append mode
//				outFile.open(filename, std::ios::app);
//
//				if (!fileExists) {
//					// File didn't exist — write header
//					outFile << "FlightId\tAverage Fuel Consumption\n";
//				}
//
//				// Write result on a new line
//				outFile << pkt.GetHeader().flightID << "\t" << avg << "\n";
//
//				outFile.close();
//			}
//		}
//
//		else {
//			std::cout << "Flight ID " << pkt.GetHeader().flightID << " completed, but not enough data to compute average.\n";
//		};
//	}
//
//	else {
//		sendMutex.lock();
//
//		memcpy(TxBuffer, &(this->FlightIdCounter), sizeof(int));
//
//		int sendSize = sendto(this->ServerSocket, TxBuffer, sizeof(int), 0, (sockaddr*)&CltAddr, sizeof(CltAddr));
//
//		/*std::cout << "First SendTo in UDPServer: " << sendSize << std::endl;*/
//
//		this->FlightIdCounter.fetch_add(1);
//
//		sendMutex.unlock();
//	};
//};
//
//
//int UDPServer::ConvertToSeconds(const TimeInfo& t) {
//	return ((t.hour * 3600) + (t.minute * 60) + t.second);
//};
//
//float UDPServer::ComputeFuelConsumption(const TimeInfo& prevTime, float prevFuel, const TimeInfo& currTime, float currFuel) {
//	int deltaT = this->ConvertToSeconds(currTime) - this->ConvertToSeconds(prevTime);
//
//	float deltaFuel = prevFuel - currFuel;
//
//	if (deltaT <= 0) {
//		return 0.0f;
//	};
//
//	return (deltaFuel / deltaT);		// This will be in units/second.
//};