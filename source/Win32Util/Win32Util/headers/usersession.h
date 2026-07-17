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

};


