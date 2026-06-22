#pragma once

class ServicesSCM
{

public:
    void printAllServiceDetails(char* format);

    void printServiceDetails(char* serviceName, char* format);

    void printServiceDescription(char* serviceName);

    void serviceInstall(char* serviceName,
        char* serviceDisplayName,
        char* serviceDescription,
        char* serviceBinaryPath);

    void serviceUnInstall(char* serviceName);

    bool serviceStart(char* serviceName);

    bool serviceStop(char* serviceName);

    bool serviceStartupTypeChange(char* serviceName, char* serviceStartType);
};
