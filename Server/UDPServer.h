#pragma once
#include <iostream>
#include<boost/asio.hpp>
#include <atomic>
#include <mutex>
#include <windows.networking.sockets.h>
#include <fstream>
#include "Packet.h"

#pragma comment(lib, "Ws2_32.lib")

const int PACKETSIZE = sizeof(Header) + sizeof(FlightData) + 4;

struct FlightSnapshot {
	TimeInfo time;
	float fuel;
};

class UDPServer {
	SOCKET ServerSocket;

	sockaddr_in SvrAddr;

	std::atomic<int> FlightIdCounter = 1;

	boost::asio::thread_pool threadPool;

	std::mutex mutex;

	std::unordered_map<int, FlightSnapshot> previousData;

	std::mutex data_mutex;

	std::unordered_map<int, std::vector<float>> flightConsumptions;

	std::mutex sendMutex;

	std::mutex fileMutex;

	int port;


	/*std::unordered_map<std::string, int> clientFlightIds;
	std::mutex clientIdMutex;*/

public:
	UDPServer(int port, int threadPoolSize);
	
	~UDPServer();

	void Start();
	
	void HandlePacket(char* Rxbuffer, sockaddr_in client_addr, bool IsPacketValid);

	int ConvertToSeconds(const TimeInfo& t);

	float ComputeFuelConsumption(const TimeInfo& prevTime, float prevFuel, const TimeInfo& currTime, float currFuel);
};