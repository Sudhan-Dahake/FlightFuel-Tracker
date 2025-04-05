#pragma once
#include <iostream>
#include<boost/asio.hpp>
#include <atomic>
#include <mutex>
#include <windows.networking.sockets.h>
#include <fstream>
#include <unordered_map>
#include <shared_mutex>
#include <thread>
#include "Packet.h"

#pragma comment(lib, "Ws2_32.lib")

const int PACKETSIZE = sizeof(Header) + sizeof(FlightData);

struct FlightSnapshot {
	int time;
	float fuel;
};

class TCPServer {
	SOCKET ServerSocket;

	sockaddr_in SvrAddr;

	std::thread flusherThread;

	std::atomic<bool> serverRunning;

	boost::asio::thread_pool threadPool;

	std::unordered_map<int, FlightSnapshot>* previousData;

	std::unordered_map<int, std::vector<float>>* flightConsumptions;

	std::mutex fileMutex;

	std::shared_mutex previousDataSharedMutex;

	std::shared_mutex flightConsumptionsSharedMutex;

	std::vector<std::pair<int, float>> avgBufferForFileWriting;

	std::mutex avgBufferForFileWritingMutex;

	int port;


public:
	TCPServer(int port, int threadPoolSize);

	~TCPServer();

	void Start();

	void HandleClient(SOCKET clientSocket);

	void HandlePacket(SOCKET clientSocket, char* Rxbuffer, bool& isClientDisconnected);

	/*int ConvertToSeconds(const TimeInfo& t);*/

	float ComputeFuelConsumption(const int& prevTime, float prevFuel, const int& currTime, float currFuel);

	void BackgroundFlusherForFile();
};