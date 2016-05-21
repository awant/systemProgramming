// clientTerminal.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define BUFSIZE 4096
#define DEFAULT_BUFLEN 4096
#define IP_PORT_LEN 16
#define EXIT_WORD "exit"
#define HELP_ARG "--help"

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "27015"
HANDLE hStdin;
bool isConnectionWithServerLost;

int connectToServer(SOCKET* ConnectSocket, PCSTR ip, PCSTR port);
DWORD WINAPI listenAndPrintThreadLoop(LPVOID lpParam);
int readStdinSendToServerLoop(SOCKET ConnectSocket);
int getInitMsgFromServer(SOCKET ConnectSocket);
void printUsage();


int main(int argc, char* argv[])
{
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;
	char* ip;
	char* port;
	HANDLE hStdin;
	isConnectionWithServerLost = false;
	// "169.254.217.250";

	if (argc == 2) {
		if (strcmp(argv[1], HELP_ARG) == 0) {
			printUsage();
			return 0;
		}

		char *p = strchr(argv[1], ':');
		if ((p == NULL) || (p == argv[1]) || (p == (argv[1]+strlen(argv[1])))) {
			return 1;
		}
		ip = (char*)calloc(sizeof(char), IP_PORT_LEN);
		port = (char*)calloc(sizeof(char), IP_PORT_LEN);
		memcpy(ip, argv[1], p - argv[1]);
		memcpy(port, p+1, argv[1] + strlen(argv[1]) - p-1);
	}
	else {
		ip = DEFAULT_IP;
		port = DEFAULT_PORT;
	}

	iResult = connectToServer(&ConnectSocket, ip, port);
	if (iResult != 0) {
		if (argc == 2) { free(ip); free(port); }
		return 1;
	}

	// Should receive init cmd.exe output from server
	iResult = getInitMsgFromServer(ConnectSocket);
	if (iResult <= 0) {
		closesocket(ConnectSocket);
		if (argc == 2) { free(ip); free(port); }
		return 1;
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
	if (iResult <= 0) {
		if (argc == 2) { free(ip); free(port); }
		return 1;
	}
	// Kill thread
	DWORD exitCode;
	if (GetExitCodeThread(hThread, &exitCode) != 0) {
		if (TerminateThread(hThread, exitCode) != 0) {
			printf("terminate thread successfully\n");
		}
	}

	// shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		if (argc == 2) { free(ip); free(port); }
		return 1;
	}

	WSACleanup();
	if (argc == 2) { free(ip); free(port); }

	printf("client closed successfully\n");
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
	return 0;
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
		else if (iResult == 0) {
			printf("Connection closed\n");
			break; 
		} else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			isConnectionWithServerLost = true;
			CancelIoEx(hStdin, NULL);
			CloseHandle(hStdin);
			hStdin = INVALID_HANDLE_VALUE;
			break;
		}
	}
	closesocket(*ConnectSocket);
	*ConnectSocket = INVALID_SOCKET;
	return 0;
}

int readStdinSendToServerLoop(SOCKET ConnectSocket)
{
	int iResult = 0;
	CHAR chBuf[BUFSIZE];
	DWORD dwRead;
	BOOL bSuccess;

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE) return -1;
	for (;;) {
		ZeroMemory(&chBuf, BUFSIZE);
		bSuccess = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL);
		if (isConnectionWithServerLost) {
			WSACleanup();
			return -1;
			break;
		}
		if (strncmp(chBuf, EXIT_WORD, strlen(EXIT_WORD)) == 0) {
			printf("goodbye...\n");
			iResult = 1;
			break;
		}
		iResult = send(ConnectSocket, chBuf, (int)strlen(chBuf), 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			if (ConnectSocket != INVALID_SOCKET)
				closesocket(ConnectSocket);
			WSACleanup();
			return -1;
		}
	}
	CloseHandle(hStdin);
	return iResult;
}

int getInitMsgFromServer(SOCKET ConnectSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	ZeroMemory(&recvbuf, DEFAULT_BUFLEN);

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

void printUsage()
{
	printf("usage: clientTerminal <ip>:<port>\n");
	printf("Connect to server. local ip with default port by default.\n");
}