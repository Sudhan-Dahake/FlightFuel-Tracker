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

bool receiveWithTimeoutAndResend(SOCKET sock, sockaddr_in& addr, Packet& packetToSend, Packet& receivedPkt, char& confirmation, char& finish)
{
    char buffer[sizeof(Packet)];
    int addrSize = sizeof(addr);
    int maxRetries = 5;
    int retries = 0;

    int Size = 0;
    char* Tx = packetToSend.SerializeData(Size);

    while (true)
    {
        //Sending an empty packet to initialize the connection with the server
        int send_result = sendto(sock, Tx, Size, 0, (sockaddr*)&addr, sizeof(addr));
        if (send_result == -1)
        {
            std::cerr << "Sending to server failed on retry " << retries + 1 << std::endl;
            return false;
        }

        std::cout << "Sent packet (attempt " << retries + 1 << ")" << std::endl;

        // Receive the data and deserialize it
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        int ready = select(0, &readfds, NULL, NULL, &timeout);
        if (ready > 0 && FD_ISSET(sock, &readfds))
        {
            int receivedBytes = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&addr, &addrSize);
            if (receivedBytes > 0)
            {
                receivedPkt = Packet(buffer);
                confirmation = receivedPkt.GetConfirmationFlag();
                finish = receivedPkt.GetFinishedFlag();
                return true;
            }
        }
        else
        {
            std::cout << "Timeout waiting for response. Retrying... (" << retries + 1 << "/" << maxRetries << ")" << std::endl;
        }

        retries++;
    }

    return false;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: client <flight_id> <input_file>" << std::endl;
        return 1;
    }

    //starts Winsock DLLs
    WSADATA wsaData;
    if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) return -1;

    //initializes socket. 
    SOCKET ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ClientSocket == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    //Connect socket to specified server
    sockaddr_in SvrAddr;
    SvrAddr.sin_family = AF_INET;						//Address family type itnernet
    SvrAddr.sin_port = htons(27000);					//port (host to network conversion)
    SvrAddr.sin_addr.s_addr = inet_addr("10.144.122.177");	//IP address

    // sending packets with the file data to the server
    std::string flightID = argv[1];
    int flightId = stoi(flightID);  // converted the string to integer for future use
    std::string fileName = argv[2];   // Getting the file name from the command line to make the solution dynamic
    std::ifstream f(fileName);

    std::cout << "This is the filename => " << fileName << std::endl;

    // The flags to determine if the packet sent to the server was corrupted
    char confirmation = 'P';
    char finish = 'N';

    if (f.is_open())
    {
        std::string InputStr = "";
        getline(f, InputStr);  //gets first line

        std::cout << "File opened" << std::endl;

        while (getline(f, InputStr))  // loop until the end of the file
        {
            std::cout << "File read" << std::endl;
            std::cout << "Confirmation Flag: " << confirmation << std::endl;
            std::cout << "This is the input string: " << InputStr << std::endl;

            if (confirmation == 'P')
            {
                FlightData flightData = readFromFile(flightId, InputStr);

                std::cout << "This is the fuel Amount: " << flightData.fuelAmount << std::endl;
                std::cout << "This is the timestamp: " << flightData.timeStamp.hour << std::endl;

                //std::cout << "Fuel Amount: " << flightData.fuelAmount << std::endl;

                //std::cout << "Time in Hours: " << flightData.timeStamp.hour << std::endl;
                //std::cout << "Time in Minutes: " << flightData.timeStamp.minute << std::endl;
                //std::cout << "Time in Seconds: " << flightData.timeStamp.second << std::endl;

                //// Setting the header information
                //newPkt.SetFlightID(flightId);
                //newPkt.SetConfirmationFlag();
                //newPkt.SetFinishedFlag();

                ///*FlightData data;*/
                //newPkt.SetData(flightData, sizeof(flightData));

                Packet packetToSend = PreparePacket(flightId, confirmation, finish, &flightData);
                Packet receivedPkt;

                if (!receiveWithTimeoutAndResend(ClientSocket, SvrAddr, packetToSend, receivedPkt, confirmation, finish))
                {
                    std::cerr << "No response from server after retries. Exiting." << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return -1;
                }

                std::cout << "Received confirmation flag: " << confirmation << std::endl;
            }

            /*
              if the previous packet was corrupted and the server set the confirmation flag to 'F' for failed,
              the send the previous packet once more
            */
            else
            {
                std::cout << "Packet is corrupted. Sending again...." << std::endl;

                Packet packetToResend = PreparePacket(flightId, confirmation, finish, nullptr, 0);
                Packet receivedPkt;

                if (!receiveWithTimeoutAndResend(ClientSocket, SvrAddr, packetToResend, receivedPkt, confirmation, finish))
                {
                    std::cerr << "No response from server after retries. Exiting." << std::endl;
                    closesocket(ClientSocket);
                    WSACleanup();
                    return -1;
                }
            }
        }

        /*Once reading from the file is over and there is no more data to transfer,
        send the final packet to disconnect from the server*/

        finish = 'D';

        Packet newPKT;

        newPKT = PreparePacket(flightId, confirmation, finish, nullptr, 0);

        int Size = 0;
        char* Tx = newPKT.SerializeData(Size);  // serializing the packet to send it to the server

        cout << "Sending the finishing packet: " << newPKT.GetFinishedFlag() << endl;

        cout << "Size of the final packet: " << Size << endl;

        //Sending a packet with header only to disconnect with the server.
        int send_result = sendto(ClientSocket, Tx, Size, 0, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));

        std::cout << "Disconnection send_result size: " << send_result << std::endl;

        // if the return value of the 'sendto' function is -1, close the server socket and end the program
        if (send_result == -1)
        {
            closesocket(ClientSocket);
            WSACleanup();
            // display an error message
            std::cout << "Sending to server failed" << std::endl;
            return -1;
        }
    }

    closesocket(ClientSocket);
    WSACleanup();
    f.close();

    return 0;
}