#pragma once

#include <windows.h>
#include <lm.h>
#include <dsgetdc.h>
#include <iostream>

#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "Advapi32.lib")


class UserManagement
{

public:
    void printDomainInfo(char* format);

    void getUserInfo(char* domainName, char* userName, char* format, bool printHeaderInVerticalFormat = true);
    void getAllUsersInfo(char* domainName, char* format);

    void verifyUserLogin(char* domainName, char* userName, char* password);

    void createLocalUser(char* userName, char* password, char* userAccountComment);
    void addUserToAdministratorGroup(char* userName);
    void removeUserFromAdministratorGroup(char* userName);
    void addUserToStandardNormalUserGroup(char* userName);
    void deleteLocalUser(char* userName);

    void setUserPassword(char* userName, char* password);
    void changeUserPassword(char* userName, char* currentPassword, char* newPassword);
    void unlockUserAcount(char* domainName, char* userName);
};
