#include "Packet.h"

Packet::Packet() {
	this->TxBuffer = nullptr;

	memset(&(this->Head), 0, sizeof(Header));

	memset(&(this->flightData), 0, sizeof(FlightData));

	this->CRC = this->CalculateCRC();
};

Packet::Packet(char* src) {
	memset(&(this->Head), 0, sizeof(Header));

	memset(&(this->flightData), 0, sizeof(FlightData));

	this->CRC = this->CalculateCRC();

	memcpy(&(this->Head), src, sizeof(Header));

	if ((this->Head.finishedFlag != 'D') && (this->Head.Length != 0)) {
		memcpy(&(this->flightData), src + sizeof(Header), this->Head.Length);
	};

	memcpy(&(this->CRC), src + sizeof(Header) + this->Head.Length, sizeof(this->CRC));

	this->TxBuffer = nullptr;
};

char* Packet::SerializeData(int& TotalSize) {
	if (this->TxBuffer) {
		delete[] TxBuffer;

		TxBuffer = nullptr;
	};

	int size = sizeof(Header) + this->Head.Length + sizeof(this->CRC);

	this->TxBuffer = new char[size];

	memset(this->TxBuffer, 0, size);

	memcpy(this->TxBuffer, &(this->Head), sizeof(Header));

	memcpy(this->TxBuffer + sizeof(Header), &(this->flightData), this->Head.Length);

	memcpy(this->TxBuffer + sizeof(Header) + this->Head.Length, &(this->CRC), sizeof(this->CRC));

	TotalSize = size;

	return this->TxBuffer;
};

Header Packet::SendConfirmation(int flightId, char confirmationFlag) {
	this->Head.confirmationFlag = confirmationFlag;
	this->Head.finishedFlag = 'N';
	this->Head.flightID = flightId;
	this->Head.Length = 0;

	return this->Head;
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

unsigned int Packet::CalculateCRC()
{
	return 0xFF00FF00;
};

bool Packet::IsBodyPresent() {
	return (this->Head.Length == 0) ? false : true;
};

bool Packet::IsFinishedFlagSet() {
	return (this->Head.finishedFlag == 'D') ? true : false;
};