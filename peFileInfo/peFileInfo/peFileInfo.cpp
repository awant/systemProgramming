// peFileInfo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <bitset>

#define HEADER_FOR_PRINT(FunctionName) \
std::cout << FunctionName << "  ---------------------------------------------------------------------------- " << std::endl;

std::string GetReadableFormatOfMachine(DWORD Machine);
std::string GetReadableSubsystemOfOptionHeader(WORD Subsystem);
std::string GetReadableMagicOfOptionHeader(WORD Magic);


// MARK: - map/unmap file

LPBYTE OpenPEFile(LPCTSTR lpszFileName) {
	HANDLE hFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
							  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cout << "invalid handle value" << std::endl;
		return NULL;
	}
	HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	CloseHandle(hFile);
	LPBYTE pBase = NULL;
	if (hMapping != NULL) {
		pBase = (LPBYTE)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
		CloseHandle(hMapping);
	}
	return pBase;
}

void ClosePEFile(LPBYTE pBase) {
	if (pBase != NULL)
		UnmapViewOfFile(pBase);
}


// MARK: - Main functions

IMAGE_NT_HEADERS* GetHeader(LPBYTE pBase) {
	if (pBase == NULL)
		return NULL;
	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)pBase;
	if (IsBadReadPtr(pDosHeader, sizeof(IMAGE_DOS_HEADER)))
		return NULL;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;
	IMAGE_NT_HEADERS* pHeader = (IMAGE_NT_HEADERS*)(pBase + pDosHeader->e_lfanew);
	if (IsBadReadPtr(pHeader, sizeof(IMAGE_NT_HEADERS)))
		return NULL;
	if (pHeader->Signature != IMAGE_NT_SIGNATURE)
		return NULL;
	return pHeader;
}

DWORD GetMachine(LPBYTE pBase) {
	IMAGE_NT_HEADERS* pHeader = GetHeader(pBase);
	IMAGE_FILE_HEADER* pFHeader = &(pHeader->FileHeader);
	return pFHeader->Machine;
}

IMAGE_SECTION_HEADER* GetSection(IMAGE_NT_HEADERS* pHeader, DWORD dwRVA) {
	if (pHeader == NULL)
		return NULL;
	IMAGE_SECTION_HEADER* pSectHeader = IMAGE_FIRST_SECTION(pHeader);
	for (UINT i = 0; i < pHeader->FileHeader.NumberOfSections; i++, pSectHeader++) {
		if (dwRVA >= pSectHeader->VirtualAddress && dwRVA < pSectHeader->VirtualAddress + pSectHeader->Misc.VirtualSize)
			return pSectHeader;
	}
	return NULL;
}

LPBYTE GetFilePointer(LPBYTE pBase, DWORD dwRVA) {
	if (pBase == NULL)
		return NULL;
	IMAGE_SECTION_HEADER* pSectHeader = GetSection(GetHeader(pBase), dwRVA);
	if (pSectHeader == NULL)
		return pBase + dwRVA;
	return pBase + pSectHeader->PointerToRawData + (dwRVA - pSectHeader->VirtualAddress);
}


// MARK: - print methods

void PrintPEHeader(IMAGE_NT_HEADERS* pHeader) {
	HEADER_FOR_PRINT("PE Header Info");
	IMAGE_FILE_HEADER* pFHeader = &(pHeader->FileHeader);
	std::cout << "Processor architecture: " << GetReadableFormatOfMachine(pFHeader->Machine) << std::endl;
	std::cout << "Number of sections: " << pFHeader->NumberOfSections << std::endl;
	std::cout << "Size of optional header: " << pFHeader->SizeOfOptionalHeader << std::endl;
	std::cout << "Characteristics: " << std::bitset<16>(pFHeader->Characteristics) << std::endl;
}

void PrintOptionalHeader(IMAGE_NT_HEADERS* pHeader) {
	HEADER_FOR_PRINT("PE Optional Header Info");
	IMAGE_OPTIONAL_HEADER* pOptHeader = &(pHeader->OptionalHeader);
	std::cout << "Magic: " << GetReadableMagicOfOptionHeader(pOptHeader->Magic) << std::endl;
	std::cout << "RVA address of entry point: " << pOptHeader->AddressOfEntryPoint << std::endl;
	std::cout << "RVA base of code: " << pOptHeader->BaseOfCode << std::endl;
#ifdef _WIN64
	std::cout << "RVA base of data: " << 0 << std::endl;
#else
	std::cout << "RVA base of data: " << GetReadableMagicOfOptionHeader(pOptHeader->BaseOfData) << std::endl;
#endif
	std::cout << "ImageBase: " << pOptHeader->ImageBase << std::endl;
	std::cout << "Subsystem Version: " << pOptHeader->MinorSubsystemVersion << "-" \
									  << pOptHeader->MajorSubsystemVersion << std::endl;
	std::cout << "Subsystem: " << GetReadableSubsystemOfOptionHeader(pOptHeader->Subsystem) << std::endl;
	IMAGE_FIRST_SECTION(pHeader);
}

void PrintSectionsInfo(IMAGE_NT_HEADERS* pHeader) {
	HEADER_FOR_PRINT("PE Sections Info");
	IMAGE_SECTION_HEADER* pSectHeader = IMAGE_FIRST_SECTION(pHeader);
	for (UINT i = 0; i < pHeader->FileHeader.NumberOfSections; i++, pSectHeader++) {
		std::cout << "Section #" << i;
		std::cout << " Name: " << pSectHeader->Name;
		std::cout << " RVA: " << std::hex << pSectHeader->VirtualAddress << std::dec << std::endl;
		std::cout << "Characteristics: " << std::bitset<32>(pSectHeader->Characteristics) << std::endl;
	}
}

void PrintExportSymbols(LPBYTE pBase) {
	HEADER_FOR_PRINT("PE Export Symbols");
	IMAGE_NT_HEADERS* pHeader = GetHeader(pBase);
	IMAGE_DATA_DIRECTORY& ExportDataDir = pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	DWORD dwExportDirStart = ExportDataDir.VirtualAddress;
	DWORD dwExportDirEnd = dwExportDirStart + ExportDataDir.Size;
	IMAGE_EXPORT_DIRECTORY* pExportDir = (IMAGE_EXPORT_DIRECTORY*)GetFilePointer(pBase, dwExportDirStart);
	LPDWORD pAddrTable = (LPDWORD)GetFilePointer(pBase, pExportDir->AddressOfFunctions);
	LPDWORD pNameTable = (LPDWORD)GetFilePointer(pBase, pExportDir->AddressOfNames);
	LPWORD pOrdTable = (LPWORD)GetFilePointer(pBase, pExportDir->AddressOfNameOrdinals);

	for (UINT i = 0; i < pExportDir->NumberOfFunctions; i++) {
		DWORD dwRVA = *pAddrTable++;
		if (dwRVA == 0)
			continue;
		std::cout << std::dec << i + pExportDir->Base << "\n";
		if (dwRVA >= dwExportDirStart && dwRVA < dwExportDirEnd)
			std::cout << "RVA of translation: " << (char*)GetFilePointer(pBase, dwRVA) << std::endl;
		else
			std::cout << "Usual RVA: " << std::hex << dwRVA << std::endl;;
		for (UINT j = 0; j < pExportDir->NumberOfNames; j++) {
			if (pOrdTable[j] == i) {
				std::cout << (char*)GetFilePointer(pBase, pNameTable[j]) << std::endl;;
				break;
			}
		}
	}
}

void PrintImportSymbols(LPBYTE pBase) {
	HEADER_FOR_PRINT("PE Import Symbols");
	IMAGE_NT_HEADERS* pHeader = GetHeader(pBase);
	IMAGE_DATA_DIRECTORY& ImportDataDir = pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	IMAGE_IMPORT_DESCRIPTOR* pImportDesc = (IMAGE_IMPORT_DESCRIPTOR*)GetFilePointer(pBase, ImportDataDir.VirtualAddress);
	IMAGE_THUNK_DATA* pINT;
	IMAGE_THUNK_DATA* pIAT;
	while (pImportDesc->Name != 0) {
		std::cout << "Lib: " << (char*)GetFilePointer(pBase, pImportDesc->Name) << std::endl;
		if (pImportDesc->OriginalFirstThunk != 0) {
			pINT = (IMAGE_THUNK_DATA*)GetFilePointer(pBase, pImportDesc->OriginalFirstThunk);
			pIAT = (IMAGE_THUNK_DATA*)GetFilePointer(pBase, pImportDesc->FirstThunk);
		}
		else {
			pINT = (IMAGE_THUNK_DATA*)GetFilePointer(pBase, pImportDesc->FirstThunk);
			pIAT = NULL;
		}
		std::cout << "function: " << std::endl;
		for (DWORD i = 0; pINT->u1.Ordinal != 0; i++) {
			if (IMAGE_SNAP_BY_ORDINAL(pINT->u1.Ordinal))
				std::cout << " | " << (pINT->u1.Ordinal & ~IMAGE_ORDINAL_FLAG) << std::endl;
			else {
				IMAGE_IMPORT_BY_NAME* p = (IMAGE_IMPORT_BY_NAME*)GetFilePointer(pBase, pINT->u1.Ordinal);
				std::cout << " | " << p->Hint << ' ' << (char*)p->Name << std::endl;
			}
			pINT++; pIAT++;
		}

		pImportDesc++;
	}
}

void PrintExceptionsSymbols(LPBYTE pBase) {
	HEADER_FOR_PRINT("PE Exceptions Symbols");
	IMAGE_NT_HEADERS* pHeader = GetHeader(pBase);
	IMAGE_DATA_DIRECTORY& ImportDataDir = pHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
	IMAGE_RUNTIME_FUNCTION_ENTRY* pImportDesc = (IMAGE_RUNTIME_FUNCTION_ENTRY*)GetFilePointer(pBase, ImportDataDir.VirtualAddress);
	while (pImportDesc->BeginAddress != 0) {
		std::cout << std::hex << "firstAddr: " << pImportDesc->BeginAddress
			<< " endAddr: " << pImportDesc->EndAddress
			<< " unwindInfoAddr: " << pImportDesc->UnwindInfoAddress << std::dec << std::endl;
		pImportDesc++;
	}

}

// MARK: - main func

int _tmain(int argc, _TCHAR* argv[])
{
	LPCTSTR path;
	if (argc != 2) {
		//path = L"C:\\Windows\\System32\\shell32.dll";
		//path = L"C:\\Windows\\splwow64.exe";
		path = L"f:\\ParallelsHW\\systemProgramming\\injection\\SimpleProgram\\x64\\Debug\\SimpleProgram.exe";
		//path = L"f:\\ParallelsHW\\systemProgramming\\peFileInfo\\open.exe";
		//path = L"f:\\ParallelsHW\\systemProgramming\\peFileInfo\\R.dll";
	}
	else {
		path = argv[1];
	}
	LPBYTE pBase = OpenPEFile(path);
	if (pBase == NULL) { return 1; }
	IMAGE_NT_HEADERS* pHeader = GetHeader(pBase);
	PrintPEHeader(pHeader);
	PrintOptionalHeader(pHeader);
	PrintSectionsInfo(pHeader);
	PrintExportSymbols(pBase);
	PrintImportSymbols(pBase);
	PrintExceptionsSymbols(pBase);
	ClosePEFile(pBase);
	return 0;
}


// MARK: - helpful methods

std::string GetReadableFormatOfMachine(DWORD Machine) {
	switch (Machine) {
	case IMAGE_FILE_MACHINE_UNKNOWN:
		return "Unknown processor";
	case IMAGE_FILE_MACHINE_I386:
		return "Intel 80386 or higher";
	case 0x014D:
		return "Intel 80486 or higher";
	case 0x014E:
		return "Intel Pentium or higher";
	case 0x0160:
		return "MIPS R3000, big-endian";
	case IMAGE_FILE_MACHINE_R3000:
		return "MIPS R3000, little-endian";
	case IMAGE_FILE_MACHINE_R4000:
		return "MIPS R4000";
	case IMAGE_FILE_MACHINE_R10000:
		return "MIPS R10000";
	case IMAGE_FILE_MACHINE_WCEMIPSV2:
		return "MIPS WCE v2";
	case IMAGE_FILE_MACHINE_ALPHA:
		return "DEC/Compaq Alpha AXP";
	case IMAGE_FILE_MACHINE_SH3:
		return "Hitachi SH3";
	case IMAGE_FILE_MACHINE_SH3DSP:
		return "Hitachi SH3 DSP";
	case IMAGE_FILE_MACHINE_SH3E:
		return "Hitachi SH3E";
	case IMAGE_FILE_MACHINE_SH4:
		return "Hitachi SH4";
	case IMAGE_FILE_MACHINE_SH5:
		return "Hitachi SH5";
	case IMAGE_FILE_MACHINE_ARM:
		return "ARM";
	case IMAGE_FILE_MACHINE_THUMB:
		return "ARM Thumb";
	case IMAGE_FILE_MACHINE_AM33:
		return "Panasonic AM33";
	case IMAGE_FILE_MACHINE_POWERPC:
		return "IBM PowerPC";
	case IMAGE_FILE_MACHINE_POWERPCFP:
		return "IBM PowerPC FP";
	case IMAGE_FILE_MACHINE_IA64:
		return "Intel IA-64 (Itanium)";
	case IMAGE_FILE_MACHINE_MIPS16:
		return "MIPS16";
	case 0x0268:
		return "Motorola 68000";
	case IMAGE_FILE_MACHINE_ALPHA64:
		return "DEC/Compaq Alpha AXP 64-bit";
	case 0x0290:
		return "HP PA-RISC";
	case IMAGE_FILE_MACHINE_MIPSFPU:
		return "MIPS with FPU";
	case IMAGE_FILE_MACHINE_MIPSFPU16:
		return "MIPS16 with FPU";
	case IMAGE_FILE_MACHINE_TRICORE:
		return "Infineon TriCore";
	case IMAGE_FILE_MACHINE_CEF:
		return "MACHINE_CEF";
	case IMAGE_FILE_MACHINE_EBC:
		return "EFI Byte Code";
	case IMAGE_FILE_MACHINE_AMD64:
		return "AMD 64 (K8)";
	case IMAGE_FILE_MACHINE_M32R:
		return "Renesas M32R";
	case IMAGE_FILE_MACHINE_CEE:
		return "MACHINE_CEE";
	}
}

std::string GetReadableSubsystemOfOptionHeader(WORD Subsystem) {
	switch (Subsystem) {
	case IMAGE_SUBSYSTEM_UNKNOWN:
		return "Unknown subsystem";
	case IMAGE_SUBSYSTEM_NATIVE:
		return "The subsystem is not required";
	case IMAGE_SUBSYSTEM_WINDOWS_GUI:
		return "The graphics subsystem Windows";
	case IMAGE_SUBSYSTEM_WINDOWS_CUI:
		return "The console subsystem of Windows";
	case IMAGE_SUBSYSTEM_OS2_CUI:
		return "Console Subsystem OS/2";
	case IMAGE_SUBSYSTEM_POSIX_CUI:
		return "Console Subsystem POSIX";
	case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:
		return "Win 9x driver";
	case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
		return "The graphics subsystem Windows CE";
	case IMAGE_SUBSYSTEM_EFI_APPLICATION:
		return "EFI program";
	case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
		return "EFI Driver, which provides boot support";
	case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
		return "EFI runtime driver";
	case IMAGE_SUBSYSTEM_EFI_ROM:
		return "Firmware ROM for UEFI";
	case IMAGE_SUBSYSTEM_XBOX:
		return "Xbox subsystem";
	}
}

std::string GetReadableMagicOfOptionHeader(WORD Magic) {
	switch (Magic) {
	case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
		return "Header firmware in ROM";
	case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
		return "PE32 header";
	case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
		return "PE32+ header";
	}
}