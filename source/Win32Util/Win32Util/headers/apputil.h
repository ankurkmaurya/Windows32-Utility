#pragma once


#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <tlhelp32.h>
#include <winternl.h>
#include <psapi.h>
#include <wchar.h>
#include <stdio.h>
#include <iomanip>
#include <iostream>
#include <strsafe.h>
#include <map>
#include <string>

#include <fstream>
#include <filesystem>
#include <chrono>
#include <sddl.h>


#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Userenv.lib")



typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
    HANDLE,
    PROCESSINFOCLASS,
    PVOID,
    ULONG,
    PULONG
    );

typedef struct _UNICODE_STRING_T
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING_T;


typedef struct _RTL_USER_PROCESS_PARAMETERS_T
{
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING_T ImagePathName;
    UNICODE_STRING_T CommandLine;
} RTL_USER_PROCESS_PARAMETERS_T;

typedef struct _PEB_T
{
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PVOID Ldr;
    RTL_USER_PROCESS_PARAMETERS_T* ProcessParameters;
} PEB_T;

typedef struct _PROCESS_BASIC_INFORMATION_T
{
    PVOID Reserved1;
    PEB_T* PebBaseAddress;
    PVOID Reserved2[2];
    ULONG_PTR UniqueProcessId;
    PVOID Reserved3;
} PROCESS_BASIC_INFORMATION_T;


namespace AppUtil {

	LPWSTR convertUTF8ToLPWSTR(char* utf8chars);

	std::string convertBSTRToString(BSTR bstr);

	bool isFileExists(const std::string& filePath);

	std::string wideStrToUTF8(const std::wstring& wstr);

	std::string WCharToUtf8(const wchar_t* wstr);

	std::string getErrorDescription(DWORD errorCode);

	std::string convertUTCToLocalTime(DWORD lastLogon);

	std::string convertSIDToString(PSID sid);

    std::wstring getProcessCommandLine(HANDLE hProcess);
    std::wstring getProcessPath(HANDLE hProcess);
    std::wstring getProcessUser(HANDLE hProcess);
}