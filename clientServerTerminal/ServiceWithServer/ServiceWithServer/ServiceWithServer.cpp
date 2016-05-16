// ServiceWithServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "svccontrol.h"
#include "svcconfig.h"

void __cdecl initSvcForSCM();
VOID SvcInstall();

TCHAR szCommand[10];
TCHAR szSvcName[80] = L"svcTerminal";

int _tmain(int argc, _TCHAR* argv[])
{
	if (lstrcmpi(argv[1], TEXT("install")) == 0) {
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
	initSvcForSCM();
	return 0;
}
