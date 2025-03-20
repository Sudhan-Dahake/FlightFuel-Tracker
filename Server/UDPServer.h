#pragma once
#include <iostream>
#include<boost/asio.hpp>
#include <atomic>
#include <mutex>
#include <windows.networking.sockets.h>
#include "Packet.h"

#pragma comment(lib, "Ws2_32.lib")

class UDPServer {
	SOCKET ServerSocket;
	sockaddr_in SvrAddr;
	std::atomic<int> FlightIdCounter;
	boost::asio::thread_pool thread_pool;
	std::mutex mutex;

public:
	UDPServer(int port, int threadPoolSize);
	
	~UDPServer();

	void Start();
	
	void HandleClient(sockaddr_in client_addr, char* buffer, int recvSize);
};