#include <windows.networking.sockets.h>
#pragma comment(lib, "Ws2_32.lib")

#include <fstream>
#include <string>
#include "Packet.h"
#include "File.h"

// Defining a function to help with setting up the packet all in once place
Packet PreparePacket(int flightId, char confirmationFlag, char finishedFlag, FlightData* data, int dataSize = sizeof(FlightData))
{
    Packet newPkt;

    // Setting the header information
    newPkt.SetFlightID(flightId);
    newPkt.SetConfirmationFlag(confirmationFlag);  //the parameter has a default value of 'P'
    newPkt.SetFinishedFlag(finishedFlag);  // the parameter has a default value of 'N'

    if (data) newPkt.SetData(*data, dataSize);  // set the value of the data which is empty for the first initializer packet
    else newPkt.SetBodyLength(dataSize);

    return newPkt;
}



int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: client <flight_id> <input_file>\n";
        return 1;
    }

    //starts Winsock DLLs
    WSADATA wsaData;
    if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) return -1;

    //initializes socket. 
    SOCKET ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    //Connect socket to specified server
    sockaddr_in SvrAddr;
    SvrAddr.sin_family = AF_INET;						//Address family type itnernet
    SvrAddr.sin_port = htons(27000);					//port (host to network conversion)
    SvrAddr.sin_addr.s_addr = inet_addr("10.144.106.227");	//IP address


    if ((connect(ClientSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR)
    {
        closesocket(ClientSocket);
        WSACleanup();
        return 0;
    }



    // sending packets with the file data to the server
    std::string flightID = argv[1];
    int flightId = stoi(flightID);  // converted the string to integer for future use
    std::string fileName = argv[2];   // Getting the file name from the command line to make the solution dynamic
    std::ifstream f(fileName);


    // The flags to determine if the packet sent to the server was corrupted
    char confirmation = 'P';
    char finish = 'N';

    if (f.is_open())
    {
        std::string InputStr = "";
        getline(f, InputStr);  //gets first line



        while (getline(f, InputStr))  // loop until the end of the file
        {
            //std::cout << "Client ID" << flightId << std::endl;

            FlightData flightData = readFromFile(flightId, InputStr);

            //std::cout << "fuel amount content before preparing: " << flightData.fuelAmount << std::endl;

            //std::cout << "Sending Packet\n";

            Packet packetToSend = PreparePacket(flightId, confirmation, finish, &flightData);


            int Size = 0;
            char* Tx = packetToSend.SerializeData(Size);



            //std::cout << "This is the Size: " << Size << std::endl;

            //std::cout << "Packet not serialized before Tx -> " << packetToSend.GetFlightID() << std::endl;
            //std::cout << "Packet not serialized before Tx, fuel Amount -> " << packetToSend.GetFlightData().fuelAmount << std::endl;
            //std::cout << "Packet not serialized before Tx, timestamp -> " << packetToSend.GetFlightData().timeStamp.hour << std::endl;

            //std::cout << "printing serialized packet " << *Tx << std::endl;


            if (send(ClientSocket, Tx, Size, 0) < 0)
            {
                std::cerr << "Error in sending the packet to server." << std::endl;
                closesocket(ClientSocket);
                WSACleanup();
                return -1;
            }

            delete[] Tx; // freeing up the memory  of Tx
            Tx = nullptr;


        }


        char finishFlag;

        FlightData flightData;

        Packet packetToSend = PreparePacket(flightId, confirmation, finishFlag = 'D', &flightData, 0);


        int Size = 0;
        char* Tx = packetToSend.SerializeData(Size);

        if (send(ClientSocket, Tx, Size, 0) < 0)
        {
            std::cerr << "Error in sending the packet to server." << std::endl;
            closesocket(ClientSocket);
            WSACleanup();
            return -1;
        }

        delete[] Tx; // freeing up the memory  of Tx
        Tx = nullptr;


    }

    closesocket(ClientSocket);
    WSACleanup();
    f.close();

    return 0;
}