#include <iostream>
#include "TCPServer.h"
#include <thread>

#define PORT 27000

int main()
{
	/*UDPServer UDPServer(27000, 4);
	UDPServer UDPServer(27000, 14);

	UDPServer.Start();*/
	int num_threads = std::thread::hardware_concurrency(); //sets number of threads based on hardware

	TCPServer TCPServer(PORT, num_threads);

	TCPServer.Start();

	return 1;
}