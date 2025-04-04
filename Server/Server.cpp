#include <iostream>
#include "TCPServer.h"
#include <thread>

int main()
{
	/*UDPServer UDPServer(27000, 4);
	UDPServer UDPServer(27000, 14);

	UDPServer.Start();*/
	int num_threads = std::thread::hardware_concurrency(); //sets number of threads based on hardware

	TCPServer TCPServer(27000, num_threads);

	TCPServer.Start();

	return 1;
}