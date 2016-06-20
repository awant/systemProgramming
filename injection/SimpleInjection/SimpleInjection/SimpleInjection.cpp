// SimpleInjection.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <iostream>

using namespace std;

#define APP_NAME L"f:\\ParallelsHW\\systemProgramming\\injection\\SimpleProgram\\x64\\Debug\\SimpleProgram.exe"

int _tmain(int argc, _TCHAR* argv[])
{
	// For Dll injection
	char szLibPath[256] = "f:\\ParallelsHW\\systemProgramming\\injection\\SimpleSumDLL\\x64\\Debug\\SimpleSumDLL.dll";
	void* pLibRemote;
	DWORD hLibModule;
	HMODULE hKernel32;
	HANDLE hThread;
	//

	STARTUPINFO cif;
	ZeroMemory(&cif, sizeof(STARTUPINFO));
	PROCESS_INFORMATION pi;
	if (CreateProcess(APP_NAME, NULL,
		NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &cif, &pi))
	{
		hKernel32 = GetModuleHandle(L"kernel32.dll");
		pLibRemote = VirtualAllocEx(pi.hProcess, NULL, sizeof(szLibPath),
			MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (pLibRemote == 0) {
			cout << "VirtualAllocEx, error: " << GetLastError() << endl;
			return 1;
		}
		if (!WriteProcessMemory(pi.hProcess, pLibRemote, (void*)szLibPath,
			sizeof(szLibPath), NULL)) {
			cout << "WriteProcessMemory, error: " << GetLastError() << endl;
			return 1;
		}
		hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) GetProcAddress(hKernel32, "LoadLibraryA"),
			pLibRemote, 0, NULL);
		if (hThread == NULL) {
			cout << "can't create remote thread" << endl;
			return 1;
		}
		WaitForSingleObject(hThread, INFINITE);
		if (!GetExitCodeThread(hThread, &hLibModule)) {
			cout << "can't exit code thread" << endl;
		}
		CloseHandle(hThread);
		VirtualFreeEx(pi.hProcess, pLibRemote, sizeof(szLibPath), MEM_RELEASE);

		hThread = CreateRemoteThread(pi.hProcess, NULL, 0,
			(LPTHREAD_START_ROUTINE) GetProcAddress(hKernel32,
			"FreeLibrary"),
			(void*)hLibModule, 0, NULL);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);

		ResumeThread(pi.hThread);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
	}
	return 0;
}

