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


enum class WindowMode
{
    Normal,
    Minimized,
    Hidden
};


struct BringWindowContext
{
    DWORD pid;
    HWND hwnd;
};


struct EnumerateWindowContext
{
    const char* format;
    bool printHeader;

    const char* key;
    const char* value;
};


class UserSession
{

  public:
    WindowMode parseWindowMode(const char* value);

	void printUserSessionDetails(char* format);
	BOOL launchProgramInActiveUserSession(char* programFilePath, char* programCommandLine, WindowMode winMode);
    
    void enumerateWindows(EnumerateWindowContext context);
    BOOL bringProcessToForeground(const char* pidValue);

    void EnableAutoLogon(const std::string& username, const std::string& password, const std::string& domain);

};


