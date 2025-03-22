#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include "Packet.h"

using namespace std;


FlightData readFromFile(int flightId, string InputStr)
{
	std::istringstream issLine(InputStr);
	string dateTime;
	string fuel;

	Packet newPkt;
	newPkt.SetFlightID(flightId);

	if (getline(issLine, dateTime, ',') && getline(issLine, fuel, ','))
	{
		FlightData flightData;

		std::istringstream ss(dateTime);
		string initalspace;
		string dateValue;
		string timeStamp;

		getline(ss, initalspace, ' ');
		getline(ss, dateValue, ' ');
		getline(ss, timeStamp);


		std::istringstream aa(timeStamp);
		string hour;
		string minute;
		string second;
		getline(aa, hour, ':');
		getline(aa, minute, ':');
		getline(aa, second);


		flightData.timeStamp.hour = stoi(hour);
		flightData.timeStamp.minute = stoi(minute);
		flightData.timeStamp.second = stoi(second);

		flightData.fuelAmount = stof(fuel);

		std::cout << flightId << " "
			<< flightData.timeStamp.hour << ":" << flightData.timeStamp.minute << ":" << flightData.timeStamp.second << " "
			<< flightData.fuelAmount << std::endl;

		return flightData;

	}

}
