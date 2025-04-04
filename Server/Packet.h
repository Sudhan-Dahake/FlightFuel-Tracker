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
	int timeStamp;
	float fuelAmount;
};

struct Header
{
	unsigned int flightID;			//Line number of the input file being transmitted
	unsigned int Length;					//Number of characters in the line
	unsigned char finishedFlag; //D for done, N for not done 
};


class Packet
{
	Header Head;
	
	FlightData flightData;

	char* TxBuffer;


public:
	Packet();		//Default Constructor - Safe State

	Packet(char* src); //deserialize

	Header SendConfirmation(int flightId, char confirmationFlag);

	char* SerializeData(int& TotalSize);

	int GetFlightId();

	Header GetHeader();

	FlightData GetFlightData();

	bool IsBodyPresent();

	bool IsFinishedFlagSet();
};