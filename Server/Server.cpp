#include <iostream>
#include "TCPServer.h"
#include <thread>
#include <Windows.h>

#define PORT 27000

// Global pointer to server instances.
TCPServer* globalServer = nullptr;

// This is a console signal handler.
// Handles cases where the terminal is closed forcefully by pressing Ctrl+C or by clicking 'X' icon.
BOOL WINAPI ConsoleHandler(DWORD signal) {
	if ((signal == CTRL_C_EVENT) || (signal == CTRL_CLOSE_EVENT)
		|| (signal == CTRL_BREAK_EVENT) || (signal == CTRL_LOGOFF_EVENT) || (signal == CTRL_SHUTDOWN_EVENT)) {
		std::cout << "\n[!] Shutdown signal received. Cleaning up...\n";

		if (globalServer) {
			delete globalServer;

			globalServer = nullptr;
		};

		ExitProcess(0);
	};

	return TRUE;
};


int main()
{
	// Registering the signal handler.
	if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		std::cout << "Failed to set control handler.\n";

		return -1;
	}

	int num_threads = std::thread::hardware_concurrency(); //sets number of threads based on hardware

	globalServer = new TCPServer(PORT, num_threads);

	globalServer->Start();


	delete globalServer;

	globalServer = nullptr;

	return 0;
}