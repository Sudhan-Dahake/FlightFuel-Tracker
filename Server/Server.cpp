#include <iostream>
#include "TCPServer.h"

int main()
{
	/*UDPServer UDPServer(27000, 4);
	UDPServer UDPServer(27000, 14);

	UDPServer.Start();*/

	TCPServer TCPServer(27000, 4);

	TCPServer.Start();

	return 1;
}