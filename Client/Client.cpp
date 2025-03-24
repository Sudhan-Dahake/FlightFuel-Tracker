#include <windows.networking.sockets.h>
#pragma comment(lib, "Ws2_32.lib")

#include <fstream>
#include <string>
#include "Packet.h"
#include "File.h"


int main(int argc, char* argv[])
{

	if (argc < 2) {
		std::cerr << "Usage: client <input_file>" << std::endl;
		return 1;
	}

	//starts Winsock DLLs
	WSADATA wsaData;
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		return -1;
	}

	//initializes socket. 
	SOCKET ClientSocket;
	ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ClientSocket == INVALID_SOCKET) {
		WSACleanup();
		return -1;
	}


	//Connect socket to specified server
	sockaddr_in SvrAddr;
	SvrAddr.sin_family = AF_INET;						//Address family type itnernet
	SvrAddr.sin_port = htons(27000);					//port (host to network conversion)
	SvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");	//IP address




	// ***** Creating packets and sending to and receiving from the server *****

	Packet newPkt;

	// Send an empty packet to the server in order to establish the relationship
	string flightID = argv[1];
	int flightId = stoi(flightID);  // converted the string to integer for future use

	// Setting the header information
	newPkt.SetFlightID(flightId);
	newPkt.SetConfirmationFlag();  //the parameter has a default value of 'P'
	newPkt.SetFinishedFlag();  // the parameter has a default value of 'N'
	newPkt.SetBodyLengthInHeader(0);


	FlightData data;
	newPkt.SetData(data, 0);  // set the value of the data which is empty for the first initializer packet
	int Size = 0;
	char* Tx = newPkt.SerializeData(Size);  // serializing the packet to send it to the server

	std::cout << "This is the size in main: " << Size << std::endl;
	//Sending an empty packet to initialize the connection with the server
	int send_result = sendto(ClientSocket, Tx, Size, 0,
		(struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

	std::cout << "Send Result in Client: " << send_result << std::endl;

	// if the return value of the 'sendto' function is -1, close the server socket and end the program
	if (send_result == -1)
	{
		closesocket(ClientSocket);
		WSACleanup();
		// display an error message
		std::cout << "Sending to server failed" << std::endl;
	}

	// Defining a buffer to store the received data in
	char buffer[sizeof(Packet)];
	Packet receivedPkt;
	int addrSize = sizeof(SvrAddr);



	//// Receive the data and deserialize it
	//int receivedBytes = recvfrom(ClientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&SvrAddr, &addrSize);
	//if (receivedBytes >= 0) {
	//	memcpy(&flightId, buffer, sizeof(int));

	//	/*Packet receivedPkt(buffer);*/   // deserialize the packet
	//}

	/*int flightId = receivedPkt.GetFlightID();*/  // saving the flight id for future use

	std::cout << "Flight Id in Client: " << flightId << std::endl;



	// sending packets with the file data to the server
	std::string fileName = argv[2];   // Getting the file name from the command line to make the solution dynamic
	std::ifstream f(fileName);

	std::cout << "This is the filename => " << fileName << std::endl;

	// The flags to determine if the packet sent to the server was corrupted
	char confirmation = 'P';
	char finish = 'N';

	if (f.is_open())
	{
		std::string InputStr = "";
		getline(f, InputStr);  //gets first line

		std::cout << "File opened" << std::endl;

		while (getline(f, InputStr))  // loop until the end of the file
		{
			std::cout << "File read" << std::endl;

			std::cout << "Confirmation Flag: " << confirmation << std::endl;

			std::cout << "This is the input string: " << InputStr << std::endl;

			if (confirmation == 'P')
			{
				FlightData flightData = readFromFile(flightId, InputStr);

				std::cout << "Fuel Amount: " << flightData.fuelAmount << std::endl;

				std::cout << "Time in Hours: " << flightData.timeStamp.hour << std::endl;
				std::cout << "Time in Minutes: " << flightData.timeStamp.minute << std::endl;
				std::cout << "Time in Seconds: " << flightData.timeStamp.second << std::endl;


				// Setting the header information
				newPkt.SetFlightID(flightId);
				newPkt.SetConfirmationFlag();
				newPkt.SetFinishedFlag();

				/*FlightData data;*/
				newPkt.SetData(flightData, sizeof(flightData));
				int Size = 0;
				char* Tx = newPkt.SerializeData(Size);  // serializing the packet to send it to the server

				std::cout << "This is inside 'confirmation == P' before sendto: " << std::endl;

				//Sending an empty packet to initialize the connection with the server
				int send_result = sendto(ClientSocket, Tx, Size, 0,
					(struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

				std::cout << "This is the send result inside 'confirmation == P': " << send_result << std::endl;

				// if the return value of the 'sendto' function is -1, close the server socket and end the program
				if (send_result == -1)
				{
					closesocket(ClientSocket);
					WSACleanup();
					// display an error message
					std::cout << "Sending to server failed" << std::endl;
				}

				// Defining a buffer to store the received data in
				char buffer[sizeof(Packet)];
				/*Packet receivedPkt;*/
				int addrSize = sizeof(SvrAddr);

				// Receive the data and deserialize it
				int receivedBytes = recvfrom(ClientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&SvrAddr, &addrSize);
				
				std::cout << "This is for the confirmation packet in client: " << receivedBytes << std::endl;

				Packet receivedPkt(buffer);

				//if (receivedBytes >= 0) {
				//	Packet receivedPkt(buffer);   // deserialize the packet
				//}

				confirmation = receivedPkt.GetConfirmationFlag();
				finish = receivedPkt.GetFinishedFlag();

				std::cout << "Confirmation Flag: " << confirmation << std::endl;
				
				std::cout << "Finished Flag: " << finish << std::endl;
			}

			/*
			  if the previous packet was corrupted and the server set the confirmation flag to 'F' for failed,
			  the send the previous packet once more
			*/
			else
			{

				std::cout << "Packet is corrupted. Sending again...." << std::endl;

				//Sending an empty packet to initialize the connection with the server
				int send_result = sendto(ClientSocket, Tx, Size, 0,
					(struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

				// if the return value of the 'sendto' function is -1, close the server socket and end the program
				if (send_result == -1)
				{
					closesocket(ClientSocket);
					WSACleanup();
					// display an error message
					std::cout << "Sending to server failed" << std::endl;
				}

				// Defining a buffer to store the received data in
				char buffer[sizeof(Packet)];
				Packet receivedPkt;
				int addrSize = sizeof(SvrAddr);

				// Receive the data and deserialize it
				int receivedBytes = recvfrom(ClientSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&SvrAddr, &addrSize);
				if (receivedBytes >= 0) {
					Packet receivedPkt(buffer);   // deserialize the packet
				}

				confirmation = receivedPkt.GetConfirmationFlag();
				finish = receivedPkt.GetFinishedFlag();
			}


		}



		/*Once reading from the file is over and there is no more data to transfer,
		send the final packet to disconnect from the server*/

		finish = 'D';

		// Setting the header information
		newPkt.SetFlightID(flightId);
		newPkt.SetConfirmationFlag();  //the parameter has a default value of 'P'
		newPkt.SetFinishedFlag(finish);  // the parameter has a default value of 'N'

		FlightData data;
		newPkt.SetData(data, 0);  // set the value of the data which is empty for the first initializer packet
		int Size = 0;
		char* Tx = newPkt.SerializeData(Size);  // serializing the packet to send it to the server


		//Sending an empty packet to initialize the connection with the server
		int send_result = sendto(ClientSocket, Tx, Size, 0,
			(struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

		std::cout << "Disconnection send_result size: " << send_result << std::endl;

		// if the return value of the 'sendto' function is -1, close the server socket and end the program
		if (send_result == -1)
		{
			closesocket(ClientSocket);
			WSACleanup();
			// display an error message
			std::cout << "Sending to server failed" << std::endl;
		}
	}


	closesocket(ClientSocket);
	WSACleanup();
	f.close();

	return 1;
}
