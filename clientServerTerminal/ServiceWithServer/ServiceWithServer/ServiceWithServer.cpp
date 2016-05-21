// ServiceWithServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "svccontrol.h"
#include "svcconfig.h"

#define HELP_ARG "--help"

void __cdecl initSvcForSCM();
VOID SvcInstall();
void printUsage();
void wchar_to_string(_TCHAR* widechar, char* str);

char serverPort[10] = "27015";
TCHAR szSvcName[80] = L"svcTerminal";
TCHAR szCommand[10];

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 1) {
		initSvcForSCM();
		return 0;
	}
	if (lstrcmpi(argv[1], TEXT(HELP_ARG)) == 0) {
		printUsage();
		return 0;
	}

	if (lstrcmpi(argv[1], TEXT("install")) == 0) {
		if (argc == 3) {
			ZeroMemory(serverPort, 10);
			wchar_to_string(argv[2], serverPort);
		}
		SvcInstall();
		return 0;
	}
	if (argc == 2) {
		StringCchCopy(szCommand, 10, argv[1]);
		if (lstrcmpi(argv[1], TEXT("start")) == 0) {
			DoStartSvc();
			return 0;
		}
		else if (lstrcmpi(argv[1], TEXT("stop")) == 0) {
			DoStopSvc();
			return 0;
		}
		else if (lstrcmpi(argv[1], TEXT("delete")) == 0) {
			DoDeleteSvc();
		}
		else {
			_tprintf(TEXT("Unknown command (%s)\n\n"), argv[1]);
			return 0;
		}
	}
	return 0;
}

void printUsage()
{
	printf("usage: ServiceWithServer [COMMAND]\n");
	printf("Run without parameters: register service\n");
	printf("COMMAND:\n");
	printf("  install <port>        install service with port, default = 27015\n");
	printf("  start                 start service\n");
	printf("  stop                  stop service\n");
	printf("  delete                delete service\n");
}

void wchar_to_string(_TCHAR* widechar, char* str)
{
	size_t size = 0;
	while (((char)widechar[size] != '\0')) {
		size++;
	}
	size++;
	size_t sizeRet = 0;
	wcstombs_s(&sizeRet, str, 10, widechar, size);
}