#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
using namespace std;


struct TimeInfo
{
	int hour;
	int minute;
	int second;
};

struct FlightData
{
	int flightId;
	TimeInfo timeStamp;
	float fuelAmount;
};

FlightData* readFromFile(int flightId, string InputStr)
{
	std::istringstream issLine(InputStr);
	string dateTime;
	string fuel;

	if (getline(issLine, dateTime, ',') && getline(issLine, fuel, ','))
	{
		FlightData flightData;
		flightData.flightId = flightId;

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

		std::cout << flightData.flightId << " "
			<< flightData.timeStamp.hour << ":" << flightData.timeStamp.minute << ":" << flightData.timeStamp.second << " "
			<< flightData.fuelAmount << std::endl;

		return &flightData;

	}

}

int getFlightID()
{
	return 0;
}
//
//int main()
//{
//	int flightId = getFlightID();
//
//	std::ifstream f("InputFile.txt");
//	if (f.is_open())
//	{
//		std::string InputStr = "";
//		getline(f, InputStr);  //gets first line
//
//		while (getline(f, InputStr))
//		{
//			FlightData* flightData = readFromFile(flightId, InputStr);
//		}
//	}
//
//	f.close();
//
//	return 1;
//}
