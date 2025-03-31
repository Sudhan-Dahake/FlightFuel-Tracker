#include <iostream>
#include "UDPServer.h"

int main()
{
	UDPServer UDPServer(27000, 14);

	UDPServer.Start();

	return 1;
}