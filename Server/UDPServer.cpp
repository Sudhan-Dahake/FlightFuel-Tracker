#include "UDPServer.h"

UDPServer::UDPServer(int port, int threadPoolSize)
	: FlightIdCounter(1), thread_pool(threadPoolSize) {
	
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

}