#pragma once
#pragma once
#include <memory>
#include <iostream>
#include <fstream>

const int EmptyPktSize = 6;					//Number of data bytes in a packet with no data field

struct TimeInfo
{
	int hour;
	int minute;
	int second;
};

struct FlightData
{
	TimeInfo timeStamp;
	float fuelAmount;
};


class Packet
{
	struct Header
	{
		unsigned int flightID;			//Line number of the input file being transmitted
		unsigned int Length;					//Number of characters in the line
		unsigned char confirmationFlag;  //P for pass, F for fail
		unsigned char finishedFlag; //D for done, N for not done 
	} Head;
	char* Data;							//The data bytes
	unsigned int CRC;					//Cyclic Redundancy Check

	char* TxBuffer;

public:
	Packet() : Data(nullptr), TxBuffer(nullptr) { memset(&Head, 0, sizeof(Head)); };		//Default Constructor - Safe State
	void SetFlightID(int value) { Head.flightID = value; };		//Sets the line number within the object

	void Display(std::ostream& os)
	{
		os << std::dec;
		os << "flight ID:  " << (int)Head.flightID << std::endl;
		os << "Length:  " << (int)Head.Length << std::endl;
		os << "Msg:     " << Data << std::endl;
		os << "CRC:     " << std::hex << (unsigned int)CRC << std::endl;
	}


	Packet(char* src) //deserialize
	{
		//if (Data)
		//	delete[] Data;

		//memcpy(&Head, src, sizeof(Head));

		//Data = new char[Head.Length];

		//memcpy(Data, src + sizeof(Head), Head.Length);

		//memcpy(&CRC, src + sizeof(Head) + Head.Length, sizeof(CRC));

		//TxBuffer = nullptr;

	}

	void SetData(char* srcData, int Size)
	{
		//if (Data)
		//	delete[] Data;

		//int actualSize = Size + 1;
		//Data = new char[actualSize];

		//memcpy(Data, srcData, actualSize);
		//Data[actualSize - 1] = '\0';

		//Head.Length = actualSize;
		//CRC = CalculateCRC();
	};

	char* SerializeData(int& TotalSize)
	{
		//if (TxBuffer)
		//	delete[] TxBuffer;

		//int size = EmptyPktSize + Head.Length;
		//TxBuffer = new char[size];

		//memcpy(TxBuffer, &Head, sizeof(Head));
		//memcpy(TxBuffer + sizeof(Head), Data, Head.Length);
		//memcpy(TxBuffer + sizeof(Head) + Head.Length, &CRC, sizeof(CRC));

		//TotalSize = size;

		//return TxBuffer;
	};

	unsigned int CalculateCRC()
	{
		return 0xFF00FF00;
	}
};
