// clientTerminal.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define BUFSIZE 4096
#define DEFAULT_BUFLEN 4096
#define EXIT_WORD "exit"


int connectToServer(SOCKET* ConnectSocket, PCSTR ip, PCSTR port);
DWORD WINAPI listenAndPrintThreadLoop(LPVOID lpParam);
int readStdinSendToServerLoop(SOCKET ConnectSocket);
int getInitMsgFromServer(SOCKET ConnectSocket);


int _tmain(int argc, char* argv[])
{
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;
	PCSTR ip = "127.0.0.1";
	PCSTR port = "27015";

	if (argc == 2) {
		char *p = strchr(argv[1], ':');
		if ((p == NULL) || (p == argv[1]) || (p == (argv[1]+strlen(argv[1])))) {
			return 1;
		}
		ip = (PSTR)malloc(sizeof(p - argv[1]));
		memcpy((void*)ip, argv[1], p - argv[1]);
		port = (PSTR)malloc(sizeof(argv[1] + strlen(argv[1]) - p));
		//memcpy((void*)ip, argv[1], p - argv[1]);

	}

	iResult = connectToServer(&ConnectSocket, ip, port);

	// Should receive init cmd.exe output from server
	getInitMsgFromServer(ConnectSocket);
	if (iResult <= 0) {
		return 0;
	}

	// Create thread for listen socket
	HANDLE hThread;
	DWORD dwThreadId;
	hThread = CreateThread(
		NULL,								// default security attributes
		0,									// use default stack size  
		listenAndPrintThreadLoop,			// thread function name
		&ConnectSocket,						// argument to thread function 
		0,									// use default creation flags 
		&dwThreadId);

	// Simultaneously send data to server
	iResult = readStdinSendToServerLoop(ConnectSocket);

	// Close all properly

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
	printf("socket closed\n");
	WSACleanup();

	return 0;
}


int connectToServer(SOCKET* ConnectSocket, PCSTR ip, PCSTR port)
{
	int iResult;
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

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
	iResult = getaddrinfo(ip, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		*ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (*ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(*ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(*ConnectSocket);
			*ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (*ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	return -1;
}

DWORD WINAPI listenAndPrintThreadLoop(LPVOID lpParam)
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
	closesocket(*ConnectSocket);
	return 0;
}

int readStdinSendToServerLoop(SOCKET ConnectSocket)
{
	int iResult = 0;
	CHAR chBuf[BUFSIZE];
	DWORD dwRead;
	BOOL bSuccess;

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE) return -1;

	for (;;) {
		ZeroMemory(&chBuf, BUFSIZE);
		bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL);
		if (strncmp(chBuf, EXIT_WORD, strlen(EXIT_WORD)) == 0) {
			printf("goodbye...\n");
			iResult = 0;
			break;
		}
		iResult = send(ConnectSocket, chBuf, (int)strlen(chBuf), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			iResult = -1;
			break;
		}
	}
	return iResult;
}

int getInitMsgFromServer(SOCKET ConnectSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	int iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		printf("%s", recvbuf);
	}
	else if (iResult == 0)
		printf("Connection closed\n");
	else
		printf("recv failed with error: %d\n", WSAGetLastError());
	return iResult;
}