// clientTerminal.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define BUFSIZE 4096
#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27015"

#define EXIT_WORD "exit"

// Thread function
DWORD WINAPI listenAndPrint(LPVOID lpParam);

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
					*ptr = NULL,
					hints;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	ZeroMemory(recvbuf, DEFAULT_BUFLEN);

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	// localhost ip address 127.0.0.1
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE) return 1;
	CHAR chBuf[BUFSIZE];
	ZeroMemory(&chBuf, BUFSIZE);
	DWORD dwRead;
	BOOL bSuccess;

	// Should receive init cmd.exe output from server
	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		printf("%s", recvbuf);
	}
	else if (iResult == 0)
		printf("Connection closed\n");
	else
		printf("recv failed with error: %d\n", WSAGetLastError());

	// Create thread for listen socket
	HANDLE hThread;
	DWORD dwThreadId;
	hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		listenAndPrint,			// thread function name
		&ConnectSocket,			// argument to thread function 
		0,                      // use default creation flags 
		&dwThreadId);

	for (;;) {
		ZeroMemory(&chBuf, BUFSIZE);
		bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL);
		if (strncmp(chBuf, EXIT_WORD, 4) == 0) {
			printf("goodbye...\n");
			break;
		}

		iResult = send(ConnectSocket, chBuf, (int)strlen(chBuf), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}
	}
	// Kill thread
	DWORD exitCode;
	if (GetExitCodeThread(hThread, &exitCode) != 0) {
		if (TerminateThread(hThread, exitCode) != 0) {
			printf("close\n");
		}
	}

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ConnectSocket);
	printf("socket closed\n");
	WSACleanup();

	return 0;
}

DWORD WINAPI listenAndPrint(LPVOID lpParam)
{
	SOCKET* ConnectSocket = (SOCKET*)lpParam;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	int bufSize = 0;
	int iResult;
	for (;;) {
		ZeroMemory(recvbuf, DEFAULT_BUFLEN);
		iResult = recv(*ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("%s", recvbuf);
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			break;
		}
	}
	return 0;
}