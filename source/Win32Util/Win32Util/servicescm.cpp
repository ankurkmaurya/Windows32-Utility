
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include "apputil.h"
#include <servicescm.h>


#pragma comment(lib, "Advapi32.lib")



LPCWSTR getServiceStateString(DWORD state)
{
    switch (state)
    {
    case SERVICE_STOPPED:
        return L"STOPPED";
    case SERVICE_START_PENDING:
        return L"START PENDING";
    case SERVICE_STOP_PENDING:
        return L"STOP PENDING";
    case SERVICE_RUNNING:
        return L"RUNNING";
    case SERVICE_CONTINUE_PENDING:
        return L"CONTINUE PENDING";
    case SERVICE_PAUSE_PENDING:
        return L"PAUSE PENDING";
    case SERVICE_PAUSED:
        return L"PAUSED";
    default:
        return L"UNKNOWN";
    }
}



LPCWSTR startTypeToString(DWORD startType)
{
    switch (startType)
    {
    case SERVICE_BOOT_START:   return L"Boot";
    case SERVICE_SYSTEM_START: return L"System";
    case SERVICE_AUTO_START:   return L"Automatic";
    case SERVICE_DEMAND_START: return L"Manual";
    case SERVICE_DISABLED:     return L"Disabled";
    default:                   return L"Unknown";
    }
}



std::wstring serviceTypeToWString(DWORD type)
{
    if (type & SERVICE_KERNEL_DRIVER)        return L"Kernel Driver";
    if (type & SERVICE_FILE_SYSTEM_DRIVER)   return L"File System Driver";

    if (type & SERVICE_WIN32_OWN_PROCESS) {
        return (type & SERVICE_INTERACTIVE_PROCESS)
            ? L"Win32 Own Process (Interactive)"
            : L"Win32 Own Process";
    }

    if (type & SERVICE_WIN32_SHARE_PROCESS) {
        return (type & SERVICE_INTERACTIVE_PROCESS)
            ? L"Win32 Shared Process (Interactive)"
            : L"Win32 Shared Process";
    }

    return L"Unknown";
}

BOOL queryServiceConfig(SC_HANDLE hSCM, LPCWSTR serviceName, LPWSTR* binaryPath, LPCWSTR* serviceStartType)
{
    SC_HANDLE hService = OpenServiceW(hSCM, serviceName, SERVICE_QUERY_CONFIG);
    if (!hService)
        return FALSE;


    DWORD bytesNeeded = 0;
    BOOL bufferSizeNeeded = QueryServiceConfigW(hService, NULL, 0, &bytesNeeded);
    if (bufferSizeNeeded == 0 && bytesNeeded == 0)
    {
        printf("Buffer Size Query failed (%lu)\n", GetLastError());
        CloseServiceHandle(hService);
        return FALSE;
    }

    QUERY_SERVICE_CONFIGW* config = (QUERY_SERVICE_CONFIGW*)malloc(bytesNeeded);
    if (!config)
    {
        printf("Buffer Allocation failed (%lu)\n", GetLastError());
        CloseServiceHandle(hService);
        return FALSE;
    }

    if (!QueryServiceConfigW(
        hService,
        config,
        bytesNeeded,
        &bytesNeeded))
    {
        printf("Query Service Config failed (%lu)\n", GetLastError());
        free(config);
        CloseServiceHandle(hService);
        return FALSE;
    }

    *binaryPath = _wcsdup(config->lpBinaryPathName);
    *serviceStartType = startTypeToString(config->dwStartType);

    free(config);
    CloseServiceHandle(hService);
    return TRUE;
}


void printServiceDetailsHorizontally(ENUM_SERVICE_STATUS_PROCESS* services, DWORD serviceCount, SC_HANDLE scHandle) {
    for (DWORD i = 0; i < serviceCount; i++)
    {
        LPCWSTR serviceStartType = NULL;
        LPWSTR binPath = NULL;
        wprintf(L"Service Name : %ls\n", services[i].lpServiceName);
        wprintf(L"Display Name : %ls\n", services[i].lpDisplayName);
        wprintf(L"Service Type : %ls\n", serviceTypeToWString(services[i].ServiceStatusProcess.dwServiceType).c_str());
        wprintf(L"State        : %ls\n", getServiceStateString(services[i].ServiceStatusProcess.dwCurrentState));

        if (queryServiceConfig(scHandle, services[i].lpServiceName, &binPath, &serviceStartType))
        {
            wprintf(L"Startup Type : %ls\n", serviceStartType);
            wprintf(L"Binary       : %ls\n", binPath);
            //free(serviceStartType);
            free(binPath);
        }
        wprintf(L"Process ID   : %lu\n", services[i].ServiceStatusProcess.dwProcessId);
        wprintf(L"\n");
    }
}


void printServiceDetailsVertically(ENUM_SERVICE_STATUS_PROCESS* services, DWORD serviceCount, SC_HANDLE scHandle) {
    wprintf(L"Service Name,Display Name,Service Type,State,Startup Type,Binary,Process ID\n");
    for (DWORD i = 0; i < serviceCount; i++)
    {
        LPCWSTR serviceStartType = NULL;
        LPWSTR binPath = NULL;
        wprintf(L"%ls,%ls,%ls,%ls,", 
            services[i].lpServiceName, 
            services[i].lpDisplayName, 
            serviceTypeToWString(services[i].ServiceStatusProcess.dwServiceType).c_str(), 
            getServiceStateString(services[i].ServiceStatusProcess.dwCurrentState));

        if (queryServiceConfig(scHandle, services[i].lpServiceName, &binPath, &serviceStartType))
        {
            wprintf(L"%ls,", serviceStartType);
            wprintf(L"%ls,", binPath);
            //free(serviceStartType);
            free(binPath);
        }
        else {
            wprintf(L",,");
        }
        wprintf(L"%lu\n", services[i].ServiceStatusProcess.dwProcessId);
    }
}


BOOL isServiceRunning(SC_HANDLE schService) {
    DWORD dwBytesNeeded;
    SERVICE_STATUS_PROCESS ssp;

    if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
        printf("Error : Service Status Query Failed.\n");
        return FALSE;
    }

    if (ssp.dwCurrentState == SERVICE_RUNNING)
    {
        printf("Service is Running.\n");
        return TRUE;
    }
    else {
        printf("Service is not Running.\n");
        return FALSE;
    }
}


void ServicesSCM::printAllServiceDetails(char* format) {

    SC_HANDLE scHandle = OpenSCManager(
        NULL,                    // local machine
        NULL,                    // SERVICES_ACTIVE_DATABASE
        SC_MANAGER_ENUMERATE_SERVICE
    );

    if (!scHandle)
    {
        printf("Error : OpenSCManager failed (%lu)\n", GetLastError());
        return;
    }

    DWORD bytesNeeded = 0;
    DWORD serviceCount = 0;
    DWORD resumeHandle = 0;

    // First call to get required buffer size
    EnumServicesStatusEx(
        scHandle,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        NULL,
        0,
        &bytesNeeded,
        &serviceCount,
        &resumeHandle,
        NULL
    );

    DWORD lastError = GetLastError();
    if (lastError != ERROR_MORE_DATA)
    {
        printf("Error : EnumServicesStatusEx failed (%lu)\n", lastError);
        CloseServiceHandle(scHandle);
        return;
    }

    BYTE* buffer = (BYTE*)malloc(bytesNeeded);
    if (!buffer)
    {
        printf("Error : Memory allocation failed.\n");
        CloseServiceHandle(scHandle);
        return;
    }

    // Second call to actually get services
    if (!EnumServicesStatusEx(
        scHandle,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        buffer,
        bytesNeeded,
        &bytesNeeded,
        &serviceCount,
        &resumeHandle,
        NULL))
    {
        printf("Error : EnumServicesStatusEx failed (%lu)\n", GetLastError());
        free(buffer);
        CloseServiceHandle(scHandle);
        return;
    }
    
    ENUM_SERVICE_STATUS_PROCESS* services = (ENUM_SERVICE_STATUS_PROCESS*)buffer;
    if (strcmp(format, "horizontal") == 0) {
        printf("Total Services: %lu\n\n", serviceCount);
        printServiceDetailsHorizontally(services, serviceCount, scHandle);
    } else if (strcmp(format, "vertical") == 0) {
        printf("Total Services: %lu\n\n", serviceCount);
        printServiceDetailsVertically(services, serviceCount, scHandle);
    } else {
        printf("Error : Found Invalid Format Type for Service Query.\n");
    }

    free(buffer);
    CloseServiceHandle(scHandle);
    return;
}



void ServicesSCM::printServiceDetails(char* serviceName, char* format)
{
    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);
    if (!serviceNameW) return;

    SC_HANDLE scHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scHandle) {
        printf("Error : OpenSCManager failed (%lu)\n", GetLastError());
        return;
    }

    SC_HANDLE schService = OpenService(
        scHandle,
        serviceNameW,
        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
    );
    if (!schService) {
        printf("Error : OpenService failed (%lu)\n", GetLastError());
        CloseServiceHandle(scHandle);
        return;
    }

    SERVICE_STATUS_PROCESS status;
    DWORD bytesNeeded = 0;
    if (!QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO,
        (LPBYTE)&status,
        sizeof(status),
        &bytesNeeded)) {
        printf("Error : QueryServiceStatusEx failed (%lu)\n", GetLastError());
    }

    // ----- Query configuration -----
    QueryServiceConfig(schService, nullptr, 0, &bytesNeeded);
    QUERY_SERVICE_CONFIG* config = (QUERY_SERVICE_CONFIG*)malloc(bytesNeeded);

    if (!QueryServiceConfig(schService, config, bytesNeeded, &bytesNeeded)) {
        printf("Error : QueryServiceConfig failed (%lu)\n", GetLastError());
        free(config);
        CloseServiceHandle(schService);
        CloseServiceHandle(scHandle);
        return;
    }

    if (strcmp(format, "horizontal") == 0) {
        wprintf(L"Service Name : %ls\n", serviceNameW);
        wprintf(L"Display Name : %ls\n", config->lpDisplayName);
        wprintf(L"Service Type : %ls\n", serviceTypeToWString(status.dwServiceType).c_str());
        wprintf(L"State        : %ls\n", getServiceStateString(status.dwCurrentState));
        wprintf(L"Startup Type : %ls\n", startTypeToString(config->dwStartType));
        wprintf(L"Log On As    : %ls\n", config->lpServiceStartName);
        wprintf(L"Binary       : %ls\n", config->lpBinaryPathName);
        wprintf(L"Process ID   : %lu\n", status.dwProcessId);
        wprintf(L"\n");
    }
    else if (strcmp(format, "vertical") == 0) {
        wprintf(L"Service Name,Display Name,Service Type,State,Startup Type,Binary,Process ID\n");
        wprintf(L"%ls,%ls,%ls,%ls,%ls,%ls,%lu",
            serviceNameW,
            config->lpDisplayName,
            serviceTypeToWString(status.dwServiceType).c_str(),
            getServiceStateString(status.dwCurrentState),
            startTypeToString(config->dwStartType),
            config->lpBinaryPathName, status.dwProcessId);
    }
    else {
        printf("Error : Found Invalid Format Type for Service Query.\n");
    }

    free(config);
    CloseServiceHandle(schService);
    CloseServiceHandle(scHandle);
}



void ServicesSCM::printServiceDescription(char* serviceName)
{
    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);
    if (!serviceNameW) return;

    SC_HANDLE scHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scHandle) {
        printf("Error : OpenSCManager failed (%lu)\n", GetLastError());
        return;
    }

    SC_HANDLE schService = OpenService(
        scHandle,
        serviceNameW,
        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
    );
    if (!schService) {
        printf("Error : OpenService failed (%lu)\n", GetLastError());
        CloseServiceHandle(scHandle);
        return;
    }

    DWORD bytesNeeded = 0;
    //get required buffer size
    QueryServiceConfig2(
        schService,
        SERVICE_CONFIG_DESCRIPTION,
        nullptr,
        0,
        &bytesNeeded
    );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        printf("Error : Buffer Size Query failed (%lu)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(scHandle);
        return;
     }

    std::vector<BYTE> buffer(bytesNeeded);
    //retrieve description
    if (!QueryServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, buffer.data(), bytesNeeded, &bytesNeeded)) {
        printf("Error : Service Config Query failed (%lu)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(scHandle);
        return;
    }

    SERVICE_DESCRIPTION* desc = reinterpret_cast<SERVICE_DESCRIPTION*>(buffer.data());
    wprintf(L"%ls\n", desc->lpDescription ? desc->lpDescription : L"");

    CloseServiceHandle(schService);
    CloseServiceHandle(scHandle);
}



void ServicesSCM::serviceInstall(char* serviceName,
                    char* serviceDisplayName, 
                    char* serviceDescription,
                    char* serviceBinaryPath)
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    //Convert UTF-8 chars to compatible wide characters
    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);
    LPWSTR serviceDisplayNameW = AppUtil::convertUTF8ToLPWSTR(serviceDisplayName);
    LPWSTR serviceBinaryPathW = AppUtil::convertUTF8ToLPWSTR(serviceBinaryPath);

    //Check if the binary file to be installed as Service exists
    std::string serviceBinaryPathStr = serviceBinaryPath ? std::string(serviceBinaryPath) : std::string();
    if (!AppUtil::isFileExists(serviceBinaryPathStr)) {
        printf("Error : Binary file path for installing Service does not exists.\n");
        return;
    }

    /*
    In case the path contains a space, it must be quoted so that it is correctly interpreted.
    For example,
    "d:\my share\myservice.exe" should be specified as ""d:\my share\myservice.exe"".
    */
    TCHAR szPath[MAX_PATH];
    StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), serviceBinaryPathW);

    // Get a handle to the SCM database. 
    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        printf("Error : OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Create the service
    schService = CreateService(
        schSCManager,              // SCM database 
        serviceNameW,               // name of service 
        serviceDisplayNameW,        // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_AUTO_START,        // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL)
    {
        printf("Error : CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return;
    }
    else {
        std::string serviceDescriptionStr = serviceDescription;
        LPSTR serviceDescriptionLP = const_cast<char*>(serviceDescriptionStr.c_str());

        SERVICE_DESCRIPTIONA serviceDescription;
        ZeroMemory(&serviceDescription, sizeof(serviceDescription));
        serviceDescription.lpDescription = serviceDescriptionLP;

        if (!ChangeServiceConfig2A(schService, SERVICE_CONFIG_DESCRIPTION, &serviceDescription)) {
            printf("Error : Service Description change failed.\n");
        }
        else {
            printf("Service Installed Successfully.\n");
        }
        delete[] serviceDescriptionLP;
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    delete[] serviceNameW;
    delete[] serviceDisplayNameW;
    delete[] serviceBinaryPathW;
}



void ServicesSCM::serviceUnInstall(char* serviceName)
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;

    //Convert UTF-8 chars to compatible wide characters
    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);

    // Get a handle to the SCM database. 
    schSCManager = OpenSCManager(
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

    if (NULL == schSCManager)
    {
        printf("Error : OpenSCManager failed (%d)\n", GetLastError());
        return;
    }

    // Get handle to the existing service. 
    schService = OpenServiceW(schSCManager, serviceNameW, SERVICE_ALL_ACCESS);
    if (schService == NULL)
    {
        printf("Error : Open Service failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return;
    }
    else {
        //Removing the Service
        SERVICE_STATUS sStatus;
        ZeroMemory(&sStatus, sizeof(sStatus));

        //First stop the service if it is Running currently
        if (isServiceRunning(schService) && !ControlService(schService, SERVICE_CONTROL_STOP, &sStatus)) {
            printf("Error : Failed to send Service Stop command.\n");
            return;
        }

        Sleep(500);
        if (sStatus.dwCurrentState == SERVICE_STOPPED || sStatus.dwCurrentState == SERVICE_STOP_PENDING)
        {
            printf("Service Stopped Successfully.\n");
        }

        Sleep(500);
        if (!DeleteService(schService)) {
            printf("Error : Failed to Remove Service.\n");
            return;
        }
        printf("Service Removed Successfully.\n");
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    delete[] serviceNameW;
}



bool ServicesSCM::serviceStart(char* serviceName)
{
    // Convert UTF-8 char* to LPWSTR
    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);
    if (!serviceNameW) return false;

    // Open Service Control Manager
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        printf("Error : OpenSCManager failed (%lu)\n", GetLastError());
        return false;
    }

    // Open the service with needed access rights
    SC_HANDLE service = OpenService(scm, serviceNameW, SERVICE_START | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (!service) {
        printf("Error : OpenService failed (%lu)\n", GetLastError());
        CloseServiceHandle(scm);
        return false;
    }

    // Check service configuration to see if it is disabled
    DWORD bytesNeeded = 0;
    QueryServiceConfig(service, nullptr, 0, &bytesNeeded); // first call to get required size
    QUERY_SERVICE_CONFIG* qsc = (QUERY_SERVICE_CONFIG*)malloc(bytesNeeded);
    if (!QueryServiceConfig(service, qsc, bytesNeeded, &bytesNeeded)) {
        printf("Error : QueryServiceConfig failed (%lu)\n", GetLastError());
        free(qsc);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }

    if (qsc->dwStartType == SERVICE_DISABLED) {
        printf("Error : Cannot start Service, Service is disabled.\n");
        free(qsc);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }
    free(qsc);

    // Check current service status
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD bytesNeededStatus = 0;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(ssStatus), &bytesNeededStatus)) {
        printf("Error : QueryServiceStatusEx failed (%lu)\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        printf("Error : Service already running.\n");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return true;
    }

    // Attempt to start the service
    if (!StartService(service, 0, nullptr)) {
        DWORD err = GetLastError();
        printf("Error : StartService failed (%lu)\n", err);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }

    // Wait until service is running
    do {
        Sleep(500);
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(ssStatus), &bytesNeededStatus)) {
            printf("Error : QueryServiceStatusEx failed (%lu)\n", GetLastError());
            break;
        }
    } while (ssStatus.dwCurrentState == SERVICE_START_PENDING);

    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
        printf("Service Started Successfully.\n");
    else
        printf("Error : Service Failed to Start.\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);

    return ssStatus.dwCurrentState == SERVICE_RUNNING;
}




bool ServicesSCM::serviceStop(char* serviceName)
{
    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);
    if (!serviceNameW) return false;

    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        printf("Error : OpenSCManager failed (%lu)\n", GetLastError());
        return false;
    }

    SC_HANDLE service = OpenService(scm, serviceNameW, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        printf("Error : OpenService failed (%lu)\n", GetLastError());
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD bytesNeeded;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(ssStatus), &bytesNeeded)) {
        printf("Error : QueryServiceStatusEx failed (%lu)\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }

    if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
        printf("Error : Service already stopped.\n");
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return true;
    }

    if (!ControlService(service, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssStatus)) {
        printf("Error : ControlService failed (%lu)\n", GetLastError());
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return false;
    }

    // Wait until stopped
    while (ssStatus.dwCurrentState != SERVICE_STOPPED) {
        Sleep(500);
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(ssStatus), &bytesNeeded)) {
            printf("Error : QueryServiceStatusEx failed (%lu)\n", GetLastError());
            break;
        }
    }
    printf("Service Stopped Successfully.\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}



bool ServicesSCM::serviceStartupTypeChange(char* serviceName, char* serviceStartType)
{
    DWORD startType = NULL;   // SERVICE_AUTO_START, SERVICE_DEMAND_START, SERVICE_DISABLED
    if (strcmp(serviceStartType, "auto") == 0) {
        startType = SERVICE_AUTO_START;
    } else if (strcmp(serviceStartType, "manual") == 0) {
        startType = SERVICE_DEMAND_START;
    } else if (strcmp(serviceStartType, "disable") == 0) {
        startType = SERVICE_DISABLED;
    } else {
        printf("Error : Received invalid service startup.type.\n");
        return false;
    }


    LPWSTR serviceNameW = AppUtil::convertUTF8ToLPWSTR(serviceName);
    if (!serviceNameW) return false;

    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        printf("Error : OpenSCManager failed (%lu)\n", GetLastError());
        return false;
    }

    SC_HANDLE service = OpenService(scm, serviceNameW, SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG);
    if (!service) {
        printf("Error : OpenService failed (%lu)\n", GetLastError());
        CloseServiceHandle(scm);
        return false;
    }

    BOOL configChanged = ChangeServiceConfig(
        service,
        SERVICE_NO_CHANGE,      // service type
        startType,              // startup type
        SERVICE_NO_CHANGE,      // error control
        nullptr,                // binary path
        nullptr,                // load order group
        nullptr,                // tag id
        nullptr,                // dependencies
        nullptr,                // account
        nullptr,                // password
        nullptr                 // display name
    );

    if (configChanged) {
        printf("Service Startup Type changed successfully.\n");
    } else {
        printf("Error : ChangeServiceConfig failed (%lu)\n", GetLastError());
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}







