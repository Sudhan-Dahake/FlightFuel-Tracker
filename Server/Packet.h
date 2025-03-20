#pragma once
#include <memory>
#include <iostream>
#include <fstream>

//const int EmptyPktSize = 14;					//Number of data bytes in a packet with no data field

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
	
	FlightData flightData;
	
	unsigned int CRC;					//Cyclic Redundancy Check

	char* TxBuffer;


public:
	Packet();		//Default Constructor - Safe State

	Packet(char* src); //deserialize

	void SendConfirmation(int flightId);

	char* SerializeData(int& TotalSize);

	unsigned int CalculateCRC();
};