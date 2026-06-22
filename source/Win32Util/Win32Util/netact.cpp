
#include <netact.h>


// TCP state strings
std::string convertTCPStateToString(DWORD state) {
    switch (state) {
        case MIB_TCP_STATE_CLOSED: return "CLOSED";
        case MIB_TCP_STATE_LISTEN: return "LISTEN";
        case MIB_TCP_STATE_SYN_SENT: return "SYN_SENT";
        case MIB_TCP_STATE_SYN_RCVD: return "SYN_RCVD";
        case MIB_TCP_STATE_ESTAB: return "ESTABLISHED";
        case MIB_TCP_STATE_FIN_WAIT1: return "FIN_WAIT1";
        case MIB_TCP_STATE_FIN_WAIT2: return "FIN_WAIT2";
        case MIB_TCP_STATE_CLOSE_WAIT: return "CLOSE_WAIT";
        case MIB_TCP_STATE_CLOSING: return "CLOSING";
        case MIB_TCP_STATE_LAST_ACK: return "LAST_ACK";
        case MIB_TCP_STATE_TIME_WAIT: return "TIME_WAIT";
        case MIB_TCP_STATE_DELETE_TCB: return "DELETE_TCB";
        default: return "UNKNOWN";
    }
}


// Convert IPv4 DWORD to string
std::string convertIPv4ToString(DWORD addr) {
    std::ostringstream ss;
    ss << (addr & 0xFF) << "."
        << ((addr >> 8) & 0xFF) << "."
        << ((addr >> 16) & 0xFF) << "."
        << ((addr >> 24) & 0xFF);
    return ss.str();
}


// Convert IPv6 address to string
std::string convertIPv6ToString(const IN6_ADDR& addr) {
    char str[46] = { 0 };
    inet_ntop(AF_INET6, (void*)&addr, str, sizeof(str));
    return std::string(str);
}


// Get services for PID (svchost)
std::vector<std::string> getServicesForPid(DWORD pid) {
    std::vector<std::string> services;

    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) return services;

    DWORD bytesNeeded = 0, servicesReturned = 0, resumeHandle = 0;

    // First call — expected to fail with ERROR_MORE_DATA
    EnumServicesStatusExA(
        scm,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_ACTIVE,
        nullptr,
        0,
        &bytesNeeded,
        &servicesReturned,
        &resumeHandle,
        nullptr
    );

    if (GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(scm);
        return services;
    }

    std::vector<BYTE> buffer(bytesNeeded);

    // IMPORTANT: reset resumeHandle
    resumeHandle = 0;

    if (EnumServicesStatusExA(
        scm,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_ACTIVE,
        buffer.data(),
        bytesNeeded,
        &bytesNeeded,
        &servicesReturned,
        &resumeHandle,
        nullptr))
    {
        auto svcArray =
            reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESSA>(buffer.data());

        for (DWORD i = 0; i < servicesReturned; ++i) {
            const auto& s = svcArray[i];

            if (s.ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING &&
                s.ServiceStatusProcess.dwProcessId == pid)
            {
                services.emplace_back(s.lpServiceName);
            }
        }
    }

    CloseServiceHandle(scm);
    return services;
}


// Print TCP row (IPv4 or IPv6) in table format
void printTcpRowInTabularFormat(bool printHeader, const std::string& proto, DWORD pid, const std::string& local, const std::string& remote,
    const std::string& state, const std::string& module, const std::string& modulepath, const std::vector<std::string>& services)
{
    if (printHeader) {
        // Print header
        std::cout << std::string(200, '-') << std::endl;
        std::cout << std::left
            << std::setw(6) << "Proto"
            << std::setw(23) << "Local Address:IP"
            << std::setw(23) << "Remote Address:IP"
            << std::setw(13) << "State"
            << std::setw(7) << "PID"
            << std::setw(28) << "Module Name"
            << std::setw(70) << "Module Path"
            << std::setw(30) << "Service"
            << std::endl;
        std::cout << std::string(200, '-') << std::endl;
    }

    std::cout << std::left
        << std::setw(6) << proto
        << std::setw(23) << local
        << std::setw(23) << remote
        << std::setw(13) << state
        << std::setw(7) << pid
        << std::setw(28) << module
        << std::setw(70) << modulepath;

    if (services.empty()) {
        std::cout << "[]";
    } 
    else {
        std::cout << "[";
        for (size_t i = 0; i < services.size(); ++i) {
            std::cout << services[i];
            if (i < services.size() - 1) std::cout << ", ";
        }
        std::cout << "]";
    }
    std::cout << std::endl;
}


// Print TCP row (IPv4 or IPv6) in horizontal format
void printTcpRowInHorizontalFormat(const std::string& proto, DWORD pid, const std::string& local, const std::string& remote,
    const std::string& state, const std::string& module, const std::string& modulepath, const std::vector<std::string>& services)
{
    std::cout << "Proto=" << proto << std::endl;
    std::cout << "Local Address:IP=" << local << std::endl;
    std::cout << "Remote Address:IP=" << remote << std::endl;
    std::cout << "State=" << state << std::endl;
    std::cout << "PID=" << pid << std::endl;
    std::cout << "Module Name=" << module << std::endl;
    std::cout << "Module Path=" << modulepath << std::endl;

    std::cout << "Service=";
    if (services.empty()) {
       std::cout << "[]";
    } 
    else {
        std::cout << "[";
        for (size_t i = 0; i < services.size(); ++i) {
            std::cout << services[i];
            if (i < services.size() - 1) std::cout << ", ";
        }
        std::cout << "]";
    }
    std::cout << std::endl;
    std::cout << std::endl;
}


// Print TCP row (IPv4 or IPv6) in vertical format
void printTcpRowInVerticalFormat(bool printHeader, const std::string& proto, DWORD pid, const std::string& local, const std::string& remote,
    const std::string& state, const std::string& module, const std::string& modulepath, const std::vector<std::string>& services)
{
    if (printHeader) {
        // Print header
        std::cout << "Proto~~";
        std::cout << "Local Address:IP~~";
        std::cout << "Remote Address:IP~~";
        std::cout << "State~~";
        std::cout << "PID~~";
        std::cout << "Module Name~~";
        std::cout << "Module Path~~";
        std::cout << "Service";
        std::cout << std::endl;
    }

    std::cout << proto << "~~";
    std::cout << local << "~~";
    std::cout << remote << "~~";
    std::cout << state << "~~";
    std::cout << pid << "~~";
    std::cout << module << "~~";
    std::cout << modulepath << "~~";

    if (services.empty()) {
        std::cout << "[]";
    }
    else {
        std::cout << "[";
        for (size_t i = 0; i < services.size(); ++i) {
            std::cout << services[i];
            if (i < services.size() - 1) std::cout << ", ";
        }
        std::cout << "]";
    }
    std::cout << std::endl;
}


// Helper for IPv4
boolean processTcpRow(bool printHeader, char* key, char* value, char* format, 
                   MIB_TCPROW_OWNER_MODULE& row, const std::string& proto) {
    boolean processTheRow = false;

    std::string localIP = convertIPv4ToString(row.dwLocalAddr);
    std::string localPort = std::to_string(ntohs((u_short)row.dwLocalPort));
    std::string remoteIP = convertIPv4ToString(row.dwRemoteAddr);
    std::string remotePort = std::to_string(ntohs((u_short)row.dwRemotePort));

    // tcp key=all/remoteip/localip/remoteport/localport value=xxxx format=vertical/horizontal/tabular"
    if (strcmp(key, "localip") == 0 && strcmp(localIP.c_str(), value) == 0) {
        processTheRow = true;
    } 
    else if (strcmp(key, "remoteip") == 0 && strcmp(remoteIP.c_str(), value) == 0) {
        processTheRow = true;
    }
    else if (strcmp(key, "localport") == 0 && strcmp(localPort.c_str(), value) == 0) {
        processTheRow = true;
    }
    else if (strcmp(key, "remoteport") == 0 && strcmp(remotePort.c_str(), value) == 0) {
        processTheRow = true;
    } 
    else if (strcmp(key, "all") == 0) {
        processTheRow = true;
    } 

    //Check if the details can be printed
    if (!processTheRow) {
        return processTheRow;
    }


    std::string local = localIP + ":" + localPort;
    std::string remote = remoteIP + ":" + remotePort;
    std::string state = convertTCPStateToString(row.dwState);

    // Module info
    std::string moduleName = "<access denied>";
    std::string modulePath = "";
    std::vector<std::string> services;
    TCPIP_OWNER_MODULE_BASIC_INFO* modInfo = nullptr;
    DWORD len = 0;
    if (GetOwnerModuleFromTcpEntry(&row, TCPIP_OWNER_MODULE_INFO_BASIC, nullptr, &len) == ERROR_INSUFFICIENT_BUFFER) {
        modInfo = (TCPIP_OWNER_MODULE_BASIC_INFO*)malloc(len);
        if (modInfo && (GetOwnerModuleFromTcpEntry(&row, TCPIP_OWNER_MODULE_INFO_BASIC, modInfo, &len) == NO_ERROR)) {
            moduleName = AppUtil::wideStrToUTF8(modInfo->pModuleName);
            modulePath = AppUtil::wideStrToUTF8(modInfo->pModulePath);
            services = getServicesForPid(row.dwOwningPid);
        }
        free(modInfo);
    }

    if (strcmp(format, "vertical") == 0) {
        printTcpRowInVerticalFormat(printHeader, proto, row.dwOwningPid, local, remote, state, moduleName, modulePath, services);
    }
    else if (strcmp(format, "horizontal") == 0) {
        printTcpRowInHorizontalFormat(proto, row.dwOwningPid, local, remote, state, moduleName, modulePath, services);
    }
    else if (strcmp(format, "tabular") == 0) {
        printTcpRowInTabularFormat(printHeader, proto, row.dwOwningPid, local, remote, state, moduleName, modulePath, services);
    }
    return processTheRow;
}


// Helper for IPv6
boolean processTcpRow(bool printHeader, char* key, char* value, char* format, 
                   MIB_TCP6ROW_OWNER_MODULE& row, const std::string& proto) {

    boolean processTheRow = false;

    std::string localIP = convertIPv6ToString(*reinterpret_cast<IN6_ADDR*>(row.ucLocalAddr));
    std::string localPort = std::to_string(ntohs((u_short)row.dwLocalPort));
    std::string remoteIP = convertIPv6ToString(*reinterpret_cast<IN6_ADDR*>(row.ucRemoteAddr));
    std::string remotePort = std::to_string(ntohs((u_short)row.dwRemotePort));

    // tcp key=all/remoteip/localip/remoteport/localport value=xxxx format=vertical/horizontal/tabular"
    if (strcmp(key, "localip") == 0 && strcmp(localIP.c_str(), value) == 0) {
        processTheRow = true;
    }
    else if (strcmp(key, "remoteip") == 0 && strcmp(remoteIP.c_str(), value) == 0) {
        processTheRow = true;
    }
    else if (strcmp(key, "localport") == 0 && strcmp(localPort.c_str(), value) == 0) {
        processTheRow = true;
    }
    else if (strcmp(key, "remoteport") == 0 && strcmp(remotePort.c_str(), value) == 0) {
        processTheRow = true;
    }
    else if (strcmp(key, "all") == 0) {
        processTheRow = true;
    }

    //Check if the details can be printed
    if (!processTheRow) {
        return processTheRow;
    }


    std::string local = localIP + ":" + localPort;
    std::string remote = remoteIP + ":" + remotePort;
    std::string state = convertTCPStateToString(row.dwState);

    std::string moduleName = "<access denied>";
    std::string modulePath = "";
    std::vector<std::string> services;
    TCPIP_OWNER_MODULE_BASIC_INFO* modInfo = nullptr;
    DWORD len = 0;
    if (GetOwnerModuleFromTcp6Entry(&row, TCPIP_OWNER_MODULE_INFO_BASIC, nullptr, &len) == ERROR_INSUFFICIENT_BUFFER) {
        modInfo = (TCPIP_OWNER_MODULE_BASIC_INFO*)malloc(len);
        if (modInfo && (GetOwnerModuleFromTcp6Entry(&row, TCPIP_OWNER_MODULE_INFO_BASIC, modInfo, &len) == NO_ERROR)) {
            moduleName = AppUtil::wideStrToUTF8(modInfo->pModuleName);
            modulePath = AppUtil::wideStrToUTF8(modInfo->pModulePath);
            services = getServicesForPid(row.dwOwningPid);

        }
        free(modInfo);
    }

    if (strcmp(format, "vertical") == 0) {
        printTcpRowInVerticalFormat(printHeader, proto, row.dwOwningPid, local, remote, state, moduleName, modulePath, services);
    }
    else if (strcmp(format, "horizontal") == 0) {
        printTcpRowInHorizontalFormat(proto, row.dwOwningPid, local, remote, state, moduleName, modulePath, services);
    }
    else if (strcmp(format, "tabular") == 0) {
        printTcpRowInTabularFormat(printHeader, proto, row.dwOwningPid, local, remote, state, moduleName, modulePath, services);
    }

    return processTheRow;
}




void NetworkActivity::printTCPNetworkActivity(char* key, char* value, char* format) {
    bool printHeader = true;

    // IPv4
    DWORD size = 0;
    GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0);
    auto table = (PMIB_TCPTABLE_OWNER_MODULE)malloc(size);
    if (GetExtendedTcpTable(table, &size, FALSE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR) {
        for (DWORD i = 0; i < table->dwNumEntries; i++) {
            boolean processTheRow = processTcpRow(printHeader, key, value, format, table->table[i], "TCP4");
            if (printHeader && processTheRow) {
                printHeader = false;
            }
        }
    }
    free(table);

    // IPv6
    size = 0;
    GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0);
    auto table6 = (PMIB_TCP6TABLE_OWNER_MODULE*)malloc(size);
    if (GetExtendedTcpTable(table6, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_MODULE_ALL, 0) == NO_ERROR) {
        for (DWORD i = 0; i < (*table6)->dwNumEntries; i++) {
            boolean processTheRow = processTcpRow(printHeader, key, value, format, (*table6)->table[i], "TCP6");
        }

    }
    free(table6);
}




