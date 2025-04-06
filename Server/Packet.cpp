#include "Packet.h"

Packet::Packet() {
	this->TxBuffer = nullptr;

	memset(&(this->Head), 0, sizeof(Header));

	memset(&(this->flightData), 0, sizeof(FlightData));
};

Packet::Packet(char* src) {
	memset(&(this->Head), 0, sizeof(Header));

	memset(&(this->flightData), 0, sizeof(FlightData));

	memcpy(&(this->Head), src, sizeof(Header));

	if ((this->Head.finishedFlag != 'D') && (this->Head.Length != 0)) {
		memcpy(&(this->flightData), src + sizeof(Header), this->Head.Length);
	};

	this->TxBuffer = nullptr;
};

int Packet::GetFlightId() {
	return this->Head.flightID;
};

Header Packet::GetHeader() {
	return this->Head;
};

FlightData Packet::GetFlightData() {
	return this->flightData;
};

bool Packet::IsBodyPresent() {
	return (this->Head.Length == 0) ? false : true;
};

bool Packet::IsFinishedFlagSet() {
	return (this->Head.finishedFlag == 'D') ? true : false;
};