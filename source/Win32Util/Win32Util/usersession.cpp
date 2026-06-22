
#include <usersession.h>
#include <apputil.h>



const wchar_t* sessionStateToString(WTS_CONNECTSTATE_CLASS state)
{
    switch (state)
    {
    case WTSActive:       return L"Active";
    case WTSConnected:    return L"Connected";
    case WTSConnectQuery: return L"ConnectQuery";
    case WTSShadow:       return L"Shadow";
    case WTSDisconnected: return L"Disconnected";
    case WTSIdle:         return L"Idle";
    case WTSListen:       return L"Listen";
    case WTSReset:        return L"Reset";
    case WTSDown:         return L"Down";
    case WTSInit:         return L"Init";
    default:              return L"Unknown";
    }
}

void UserSession::printUserSessionDetails(char* format) {
    PWTS_SESSION_INFO pSessions = NULL;
    DWORD count = 0;

    if (!WTSEnumerateSessionsW(
        WTS_CURRENT_SERVER_HANDLE,
        0,
        1,
        &pSessions,
        &count))
    {
        printf("Error : WTSEnumerateSessions failed. Code=%lu\n", GetLastError());
        return;
    }

    BOOL printHeader = TRUE;
    for (DWORD i = 0; i < count; i++)
    {
        DWORD sessionId = pSessions[i].SessionId;

        LPWSTR user = NULL;
        LPWSTR domain = NULL;
        LPWSTR client = NULL;

        DWORD bytes = 0;

        WTSQuerySessionInformationW(
            WTS_CURRENT_SERVER_HANDLE,
            sessionId,
            WTSUserName,
            &user,
            &bytes);

        WTSQuerySessionInformationW(
            WTS_CURRENT_SERVER_HANDLE,
            sessionId,
            WTSDomainName,
            &domain,
            &bytes);

        WTSQuerySessionInformationW(
            WTS_CURRENT_SERVER_HANDLE,
            sessionId,
            WTSClientName,
            &client,
            &bytes);

        
        std::wstring fullUser;
        if (domain && *domain)
        {
            fullUser = domain;
            if (user && *user)
            {
                fullUser += L"\\";
                fullUser += user;
            }
        }
        else if (user && *user)
        {
            fullUser = user;
        }
        else {
            fullUser = L" ";
        }

        if (strcmp(format, "vertical") == 0) {
            if (printHeader)
            {
                // Print Header
                std::cout << "SessionId~~";
                std::cout << "State~~";
                std::cout << "WinStation~~";
                std::cout << "User~~";
                std::cout << "Client";
                std::cout << std::endl;

                printHeader = FALSE;
            }

            // Print Body
            std::cout << sessionId << "~~";
            std::cout << AppUtil::wideStrToUTF8(sessionStateToString(pSessions[i].State)) << "~~";
            std::cout << AppUtil::wideStrToUTF8((pSessions[i].pWinStationName ? pSessions[i].pWinStationName : L" ")) << "~~";
            std::cout << AppUtil::wideStrToUTF8(fullUser) << "~~";
            std::cout << AppUtil::WCharToUtf8(client ? client : L" ");
            std::cout << std::endl;
        }
        else if (strcmp(format, "horizontal") == 0) {
            printf(
                "SessionId  : %lu\n"
                "State      : %s\n"
                "WinStation : %s\n"
                "User       : %s\n"
                "Client     : %s\n\n",
                sessionId,
                AppUtil::wideStrToUTF8(sessionStateToString(pSessions[i].State)).c_str(),
                AppUtil::wideStrToUTF8(pSessions[i].pWinStationName? pSessions[i].pWinStationName: L" ").c_str(),
                AppUtil::wideStrToUTF8(fullUser).c_str(),
                AppUtil::wideStrToUTF8(client ? client : L" ").c_str());
        }
        else if (strcmp(format, "tabular") == 0) {
            if (printHeader)
            {
                std::cout
                    << std::left
                    << std::setw(12) << "SessionId"
                    << std::setw(16) << "State"
                    << std::setw(20) << "WinStation"
                    << std::setw(40) << "User"
                    << std::setw(20) << "Client"
                    << std::endl;

                std::cout
                    << std::string(108, '-')
                    << std::endl;

                printHeader = FALSE;
            }

            std::cout
                << std::left
                << std::setw(12) << sessionId
                << std::setw(16) << AppUtil::wideStrToUTF8(sessionStateToString(pSessions[i].State))
                << std::setw(20) << AppUtil::wideStrToUTF8(pSessions[i].pWinStationName? pSessions[i].pWinStationName: L" ")
                << std::setw(40) << AppUtil::wideStrToUTF8(fullUser)
                << std::setw(20) << AppUtil::wideStrToUTF8(client ? client : L" ")
                << std::endl;
        }

        if (user)   WTSFreeMemory(user);
        if (domain) WTSFreeMemory(domain);
        if (client) WTSFreeMemory(client);
    }
    WTSFreeMemory(pSessions);

}


WindowMode UserSession::parseWindowMode(const char* value)
{
    if (!value)
        return WindowMode::Normal;

    if (_stricmp(value, "hidden") == 0)
    {
        return WindowMode::Hidden;
    }

    if (_stricmp(value, "minimized") == 0)
    {
        return WindowMode::Minimized;
    }

    return WindowMode::Normal;
}

BOOL UserSession::launchProgramInActiveUserSession(char* programFilePath, char* programCommandLineArguments, WindowMode winMode)
{
    //Check if the binary file to be started exists or not in the system
    std::string programBinaryPathStr = programFilePath ? std::string(programFilePath) : std::string();
    if (!AppUtil::isFileExists(programBinaryPathStr)) {
        printf("Error : Given file path for launching the Program does not exists.\n");
        return FALSE;
    }

    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF)
    {
        std::cerr << "Error : No active console session found." << std::endl;
        return FALSE;
    }

    HANDLE hUserToken = NULL;
    HANDLE hPrimaryToken = NULL;
    LPVOID pEnvironment = NULL;

    // Get active user's token
    if (!WTSQueryUserToken(sessionId, &hUserToken))
    {
        std::cerr << "Error : WTSQueryUserToken failed. Code=" << GetLastError() << std::endl;
        return FALSE;
    }

    // Create primary token
    if (!DuplicateTokenEx(
        hUserToken,
        MAXIMUM_ALLOWED,
        NULL,
        SecurityImpersonation,
        TokenPrimary,
        &hPrimaryToken))
    {
        std::cerr << "Error : DuplicateTokenEx failed. Code=" << GetLastError() << std::endl;
        CloseHandle(hUserToken);
        return FALSE;
    }

    // Create user environment
    if (!CreateEnvironmentBlock(
        &pEnvironment,
        hPrimaryToken,
        FALSE))
    {
        std::cerr << "Error : CreateEnvironmentBlock failed. Code=" << GetLastError() << std::endl;
        pEnvironment = NULL;
    }

    DWORD creationFlags = CREATE_UNICODE_ENVIRONMENT;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);

    // Interactive desktop
    si.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");

    switch (winMode)
    {
        case WindowMode::Normal:
            break;
        case WindowMode::Minimized:
            creationFlags |= CREATE_NEW_CONSOLE;
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOWMINIMIZED;
            break;
        case WindowMode::Hidden:
            creationFlags |= CREATE_NO_WINDOW;
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            break;
    }

    PROCESS_INFORMATION pi = {};

    LPWSTR programBinaryPathW = AppUtil::convertUTF8ToLPWSTR(programFilePath);
    LPWSTR programCommandLineW = AppUtil::convertUTF8ToLPWSTR(programCommandLineArguments);
    std::cout << "Program Path : " << programFilePath << std::endl;
    std::cout << "Command Line : " << programCommandLineArguments << std::endl;

    //Build the CommandLine
    std::wstring cmdLine;
    cmdLine += L"\"";
    if (programBinaryPathW)
    {
        cmdLine += programBinaryPathW;
    }
    cmdLine += L"\" ";
    if (programCommandLineW)
    {
        cmdLine += programCommandLineW;
    }

    std::vector<wchar_t> cmd(cmdLine.begin(), cmdLine.end());
    cmd.push_back(L'\0');

    BOOL result = CreateProcessAsUserW(
        hPrimaryToken,
        NULL,               // Application name
        cmd.data(),        // Command line
        NULL,
        NULL,
        FALSE,
        creationFlags,
        pEnvironment,
        NULL,
        &si,
        &pi);

    if (!result)
    {
        std::cerr << "Error : CreateProcessAsUser failed. Code=" << GetLastError() << std::endl;
    }
    else
    {
        DWORD processSessionId = 0;

        if (ProcessIdToSessionId(
            pi.dwProcessId,
            &processSessionId))
        {
            std::cout << "Program Started, PID=" << pi.dwProcessId << " in Session=" << processSessionId << std::endl;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    if (pEnvironment) {
        DestroyEnvironmentBlock(pEnvironment);
    }

    CloseHandle(hPrimaryToken);
    CloseHandle(hUserToken);

    if (programBinaryPathW)
        delete[] programBinaryPathW;

    if (programCommandLineW)
        delete[] programCommandLineW;

    return result;
}




static std::wstring GetProcessPath(HANDLE hProcess)
{
    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;

    if (QueryFullProcessImageNameW(
        hProcess,
        0,
        path,
        &size))
    {
        return path;
    }

    return L"";
}


static std::wstring GetProcessCommandLine(HANDLE hProcess)
{
    std::wstring result;

    if (!hProcess)
        return result;

    HMODULE hNtDll =
        GetModuleHandleW(L"ntdll.dll");

    if (!hNtDll)
        return result;

    auto NtQueryInformationProcess =
        (_NtQueryInformationProcess)
        GetProcAddress(
            hNtDll,
            "NtQueryInformationProcess");

    if (!NtQueryInformationProcess)
        return result;

    PROCESS_BASIC_INFORMATION_T pbi = { 0 };

    ULONG retLen = 0;

    NTSTATUS status =
        NtQueryInformationProcess(
            hProcess,
            ProcessBasicInformation,
            &pbi,
            sizeof(pbi),
            &retLen);

    if (status != 0)
        return result;

    PEB_T peb = { 0 };

    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(
        hProcess,
        pbi.PebBaseAddress,
        &peb,
        sizeof(peb),
        &bytesRead))
    {
        return result;
    }

    RTL_USER_PROCESS_PARAMETERS_T procParams = { 0 };

    if (!ReadProcessMemory(
        hProcess,
        peb.ProcessParameters,
        &procParams,
        sizeof(procParams),
        &bytesRead))
    {
        return result;
    }

    USHORT byteLen =
        procParams.CommandLine.Length;

    if (byteLen == 0)
        return result;

    // Safety limit
    if (byteLen > 32766)
        return result;

    SIZE_T wcharLen =
        byteLen / sizeof(wchar_t);

    if (wcharLen == 0)
        return result;

    std::vector<wchar_t> buffer(
        wcharLen + 1,
        L'\0');

    if (!ReadProcessMemory(
        hProcess,
        procParams.CommandLine.Buffer,
        buffer.data(),
        byteLen,
        &bytesRead))
    {
        return result;
    }

    buffer[wcharLen] = L'\0';

    result.assign(buffer.data());

    return result;
}


static std::wstring GetProcessUser(HANDLE hProcess)
{
    HANDLE hToken = NULL;

    if (!OpenProcessToken(
        hProcess,
        TOKEN_QUERY,
        &hToken))
    {
        return L"";
    }

    DWORD len = 0;

    GetTokenInformation(
        hToken,
        TokenUser,
        NULL,
        0,
        &len);

    BYTE* buffer = new BYTE[len];

    if (!GetTokenInformation(
        hToken,
        TokenUser,
        buffer,
        len,
        &len))
    {
        delete[] buffer;
        CloseHandle(hToken);
        return L"";
    }

    TOKEN_USER* tu = (TOKEN_USER*)buffer;

    wchar_t name[256];
    wchar_t domain[256];

    DWORD nameLen = 256;
    DWORD domainLen = 256;

    SID_NAME_USE sidType;

    LookupAccountSidW(
        NULL,
        tu->User.Sid,
        name,
        &nameLen,
        domain,
        &domainLen,
        &sidType);

    std::wstring result = std::wstring(domain) + L"\\" + name;

    delete[] buffer;

    CloseHandle(hToken);

    return result;
}


static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    EnumerateWindowContext* enumerateContext = (EnumerateWindowContext*)lParam;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) {
        return TRUE;
    }
       
    //Filter based on pid key
    if (strcmp(enumerateContext->key, "pid") == 0) {
        DWORD targetPid = static_cast<DWORD>(strtoul(enumerateContext->value, NULL, 10));
        if (pid != targetPid) {
            return TRUE;
        }
    }
  

    HANDLE hProcess = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            FALSE,
            pid);

    if (!hProcess)
        return TRUE;


    wchar_t title[512];
    GetWindowTextW(hwnd, title, 512);
    std::string titleUTF8 = AppUtil::WCharToUtf8(title);

    bool visible = IsWindowVisible(hwnd);
    bool minimized = IsIconic(hwnd);

    HWND fg = GetForegroundWindow();

    std::string windowState;
    const char* winState;
    if (GetAncestor(fg, GA_ROOT) == GetAncestor(hwnd, GA_ROOT)) {
        windowState = "FOREGROUND";
        winState = "foreground";
    }
    else if (minimized) {
        windowState = "MINIMIZED";
        winState = "minimized";
    }
    else if (visible) {
        windowState = "VISIBLE";
        winState = "visible";
    }
    else {
        windowState = "HIDDEN";
        winState = "hidden";
    }
        
    //Filter based on windowstate key
    if (strcmp(enumerateContext->key, "windowstate") == 0) {
        if (strcmp(enumerateContext->value, winState) != 0) {
            CloseHandle(hProcess);
            return TRUE;
        }
    }

    DWORD sessionId = 0;
    ProcessIdToSessionId(pid, &sessionId);
    //Filter based on sessionid key
    if (strcmp(enumerateContext->key, "sessionid") == 0) {
        DWORD targetSessionId = static_cast<DWORD>(strtoul(enumerateContext->value, NULL, 10));
        if (sessionId != targetSessionId)
        {
            CloseHandle(hProcess);
            return TRUE;
        }
    }

    std::cout << std::endl;
    if (strcmp(enumerateContext->format, "vertical") == 0) {
        if (enumerateContext->printHeader)
        {
            // Print Header
            std::cout << "PID~~";
            std::cout << "SessionId~~";
            std::cout << "User~~";
            std::cout << "Window State~~";
            std::cout << "Title~~";
            std::cout << "Path~~";
            std::cout << "Command Line~~";
            std::cout << "Handle";
            std::cout << std::endl;

            enumerateContext->printHeader = FALSE;
        }

        // Print Body
        std::cout
            << pid << "~~"
            << sessionId << "~~"
            << AppUtil::wideStrToUTF8(GetProcessUser(hProcess)) << "~~"
            << windowState << "~~"
            << titleUTF8 << "~~"
            << AppUtil::wideStrToUTF8(GetProcessPath(hProcess)) << "~~"
            << AppUtil::wideStrToUTF8(GetProcessCommandLine(hProcess)) << "~~"
            << hwnd
            << "\n";

    }
    else if (strcmp(enumerateContext->format, "horizontal") == 0) {
        std::cout
            << "PID: " << pid
            << "\nSessionId: " << sessionId
            << "\nUser: " << AppUtil::wideStrToUTF8(GetProcessUser(hProcess))
            << "\nWindow State: " << windowState
            << "\nTitle: " << titleUTF8
            << "\nPath: " << AppUtil::wideStrToUTF8(GetProcessPath(hProcess))
            << "\nCommand Line: " << AppUtil::wideStrToUTF8(GetProcessCommandLine(hProcess))
            << "\nHandle: " << hwnd
            << "\n\n";
    }


    CloseHandle(hProcess);
    return TRUE;
}


void UserSession::enumerateWindows(EnumerateWindowContext context)
{
    EnumWindows(EnumWindowsProc, (LPARAM)&context);
}



static BOOL CALLBACK FindProcessWindowProc(HWND hwnd, LPARAM lParam)
{
    BringWindowContext* ctx = (BringWindowContext*)lParam;

    if (!IsWindow(hwnd))
    {
        return TRUE;
    }

    if (GetWindowTextLengthW(hwnd) == 0)
    {
        return TRUE;
    }

    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);

    if (windowPid != ctx->pid)
    {
        return TRUE;
    }

    // Ignore invisible windows
    if (!IsWindowVisible(hwnd))
    {
        return TRUE;
    }

    // Ignore owned/helper windows
    if (GetWindow(hwnd, GW_OWNER) != NULL)
    {
        return TRUE;
    }

    // Ignore tool windows
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)
    {
        return TRUE;
    }


    wchar_t title[512] = { 0 };
    GetWindowTextW(hwnd,title,512);
    // Ignore untitled windows
    if (wcslen(title) == 0)
    {
        return TRUE;
    }

    ctx->hwnd = hwnd;

    // Stop enumeration
    return FALSE;
}



BOOL UserSession::bringProcessToForeground(const char* pidValue)
{
    DWORD pid = static_cast<DWORD>(strtoul(pidValue, NULL, 10));

    BringWindowContext ctx = { 0 };
    ctx.pid = pid;
    ctx.hwnd = NULL;

    EnumWindows( FindProcessWindowProc, (LPARAM)&ctx);

    if (!ctx.hwnd)
    {
        std::cerr << "Error : No suitable window found for PID="  << pid << std::endl;
        return FALSE;
    }

    HWND hwnd = ctx.hwnd;

    // Restore minimized window
    if (IsIconic(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }
    else
    {
        ShowWindow(hwnd, SW_SHOW);
    }

    // Force window to top
    BringWindowToTop(hwnd);
    HWND foregroundWindow = GetForegroundWindow();
    DWORD foregroundThread =  GetWindowThreadProcessId(foregroundWindow, NULL);

    DWORD targetThread = GetWindowThreadProcessId(hwnd, NULL);

    // Attach input queues
    AttachThreadInput(foregroundThread, targetThread, TRUE);

    // Make window topmost temporarily
    SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    SetActiveWindow(hwnd);

    AttachThreadInput(foregroundThread, targetThread, FALSE);

    std::cout << "Process brought to foreground. PID=" << pid << std::endl;

    return TRUE;
}



