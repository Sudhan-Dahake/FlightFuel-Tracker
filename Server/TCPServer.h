#pragma once
#include <iostream>
#include<boost/asio.hpp>
#include <atomic>
#include <mutex>
#include <windows.networking.sockets.h>
#include <fstream>
#include <unordered_map>
#include "Packet.h"

#pragma comment(lib, "Ws2_32.lib")

const int PACKETSIZE = sizeof(Header) + sizeof(FlightData) + 4 + 2;

struct FlightSnapshot {
	int time;
	float fuel;
};

class TCPServer {
	SOCKET ServerSocket;

	sockaddr_in SvrAddr;

	boost::asio::thread_pool threadPool;

	std::mutex mutex;

	std::unordered_map<int, FlightSnapshot>* previousData;

	std::mutex data_mutex;

	std::unordered_map<int, std::vector<float>>* flightConsumptions;

	std::mutex sendMutex;

	std::mutex fileMutex;

	int port;


public:
	TCPServer(int port, int threadPoolSize);

	~TCPServer();

	void Start();

	void HandleClient(SOCKET clientSocket);

	void HandlePacket(SOCKET clientSocket, char* Rxbuffer, bool& isClientDisconnected);

	int ConvertToSeconds(const TimeInfo& t);

	float ComputeFuelConsumption(const int& prevTime, float prevFuel, const int& currTime, float currFuel);
};