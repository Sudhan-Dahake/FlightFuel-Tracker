#include "Packet.h"

Packet::Packet() {
	this->TxBuffer = nullptr;

	memset(&(this->Head), 0, sizeof(Header));

	memset(&(this->flightData), 0, sizeof(FlightData));
};

Packet::Packet(char* src) {
	memset(&(this->Head), 0, sizeof(Header));

	memcpy(&(this->Head), src, sizeof(Header));

	memset(&(this->flightData), 0, sizeof(FlightData));

	memcpy(&(this->flightData), src + sizeof(Header), this->Head.Length);

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

unsigned int Packet::CalculateCRC()
{
	return 0xFF00FF00;
};