#include <windows.networking.sockets.h>
#include <iostream>
#include "Packet.h"

#pragma comment(lib, "Ws2_32.lib")

int main()
{
	WSADATA wsaData;
	SOCKET ServerSocket, ConnectionSocket;
	sockaddr_in SvrAddr;

	WSAStartup(MAKEWORD(2, 2), &wsaData);
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == SOCKET_ERROR)
		return -1;

	SvrAddr.sin_family = AF_INET;
	SvrAddr.sin_addr.s_addr = INADDR_ANY;
	SvrAddr.sin_port = htons(27001);
	bind(ServerSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

	if (ServerSocket == SOCKET_ERROR)
		return -1;

	listen(ServerSocket, 1);
	std::cout << "Waiting for client connection\n" << std::endl;
	ConnectionSocket = SOCKET_ERROR;
	ConnectionSocket = accept(ServerSocket, NULL, NULL);

	if (ConnectionSocket == SOCKET_ERROR)
		return -1;

	std::cout << "Connection Established" << std::endl;

	//BAD Practice, but needed for this assignment
	//Loop forever, until terminated with CTRL-C
	while (true)
	{
		char Rx[50];
		memset(Rx, 0, sizeof(Rx));

		recv(ConnectionSocket, Rx, sizeof(Rx), 0);
		/*DataPacket RxPkt;
		memcpy((char*)&RxPkt, Rx, sizeof(DataPacket));*/

		Packet* RxPkt = new Packet[sizeof(Packet)];

		memcpy(RxPkt, Rx, sizeof(DataPacket));

		cout << "PktID: " << RxPkt->PktNumber << " Data: " << RxPkt->Data << endl;

		send(ConnectionSocket, "OK", sizeof("OK"), 0);
	}

	closesocket(ConnectionSocket);	//closes incoming socket
	closesocket(ServerSocket);	    //closes server socket	
	WSACleanup();					//frees Winsock resources

	return 1;
}