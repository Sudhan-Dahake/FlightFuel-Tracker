#pragma once
#pragma once
#include <memory>
#include <iostream>
#include <fstream>

//const int EmptyPktSize = 6;					//Number of data bytes in a packet with no data field

struct TimeInfo
{
	int hour;
	int minute;
	int second;
};


struct FlightData
{
	int timeStamp;
	float fuelAmount;
};

class Packet
{
	struct Header
	{
		unsigned int flightID;			//Line number of the input file being transmitted
		unsigned int Length;					//Number of characters in the line
		unsigned char finishedFlag; //D for done, N for not done 
	} Head;
	FlightData Data;							//The data bytes

	char* TxBuffer;

public:
	Packet() : TxBuffer(nullptr), Data{} { memset(&Head, 0, sizeof(Head)); };		//Default Constructor - Safe State

	void Display(std::ostream& os)
	{
		os << std::dec;
		os << "flight ID:  " << (int)Head.flightID << std::endl;
		os << "Length:  " << (int)Head.Length << std::endl;
		os << "Timestamp: " << Data.timeStamp << std::endl;//Data.timeStamp.hour << Data.timeStamp.minute << Data.timeStamp.second << std::endl;
		os << "Fuel amount: " << Data.fuelAmount << std::endl;		
	}


	Packet(char* src) //deserialize
	{
		memcpy(&Head, src, sizeof(Head));

		/*std::cout << "Confirmation Flag Inside Packet.h: " << Head.confirmationFlag << std::endl;

		std::cout << "Finished Flag Inside Packet.h: " << Head.finishedFlag << std::endl;*/

		if (this->Head.Length != 0) {
			memcpy(&Data, src + sizeof(Head), Head.Length);
		};
	}

	void SetData(FlightData& srcData, int Size)
	{
		Data = srcData;
		Head.Length = Size;   //updating the header information
	};

	char* SerializeData(int& TotalSize)
	{
		// cleaning up the old memory
		if (TxBuffer)
			delete[] TxBuffer;

		/*std::cout << "Size of HEADER: " << sizeof(Header) << std::endl;
		std::cout << "Size of Head: " << sizeof(Head) << std::endl;
		std::cout << "Size of FlightData: " << sizeof(FlightData) << std::endl;
		std::cout << "Size of CRC: " << sizeof(CRC) << std::endl;*/

		TotalSize = sizeof(Header) + Head.Length; //+ sizeof(CRC);  //this is the full size of the packet (head + body + tail)

		/*std::cout << TotalSize << " => This is the size of total size." << std::endl;*/

		TxBuffer = new char[TotalSize];

		memcpy(TxBuffer, &Head, sizeof(Head));

		if (Head.Length != 0) {
			memcpy(TxBuffer + sizeof(Head), &Data, Head.Length);
		}



		return TxBuffer;
	};



	// Defining the getter and setter for the flight ID
	unsigned int GetFlightID() { return Head.flightID; }
	void SetFlightID(unsigned int flightID) { Head.flightID = flightID; }

	//// Defining a setter and getter for the confirmation flag (to let the client know if there was a problem with the packet sent to server)
	//unsigned char GetConfirmationFlag() { return Head.confirmationFlag; }
	//// Set a default value of P (pass) for the flag
	//void SetConfirmationFlag(unsigned char confirmation = 'P') { Head.confirmationFlag = confirmation; }

	// Defining a setter and getter for the finished flag (to let the server know when the client is done)
	unsigned char GetFinishedFlag() { return Head.finishedFlag; }
	// Set a default value of N (Not done) for the flag
	void SetFinishedFlag(unsigned char status = 'N') { Head.finishedFlag = status; }

	void SetBodyLength(unsigned int bodyLength = 0) { Head.Length = bodyLength; };




	//void SetBodyLengthInHeader(unsigned int bodyLength) { Head.Length = bodyLength; };
};
