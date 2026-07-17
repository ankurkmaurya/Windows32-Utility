
#include <usermanagement.h>
#include <apputil.h>



static std::string passwordExpiredToString(DWORD passwordExpired)
{
    switch (passwordExpired)
    {
    case 0:
        return "No";

    case 1:
        return "Yes";

    default:
        return "Unknown";
    }
}


static std::vector<std::string> userFlagsToText(DWORD flags)
{
    std::vector<std::string> result;

    if (flags & UF_SCRIPT)
        result.push_back("UF_SCRIPT");

    if (flags & UF_ACCOUNTDISABLE)
        result.push_back("UF_ACCOUNTDISABLE (Account Disabled)");

    if (flags & UF_HOMEDIR_REQUIRED)
        result.push_back("UF_HOMEDIR_REQUIRED");

    if (flags & UF_LOCKOUT)
        result.push_back("UF_LOCKOUT (Account Locked)");

    if (flags & UF_PASSWD_NOTREQD)
        result.push_back("UF_PASSWD_NOTREQD (No Password Required)");

    if (flags & UF_PASSWD_CANT_CHANGE)
        result.push_back("UF_PASSWD_CANT_CHANGE");

    if (flags & UF_NORMAL_ACCOUNT)
        result.push_back("UF_NORMAL_ACCOUNT (Normal User)");

    if (flags & UF_TEMP_DUPLICATE_ACCOUNT)
        result.push_back("UF_TEMP_DUPLICATE_ACCOUNT");

    if (flags & UF_INTERDOMAIN_TRUST_ACCOUNT)
        result.push_back("UF_INTERDOMAIN_TRUST_ACCOUNT");

    if (flags & UF_WORKSTATION_TRUST_ACCOUNT)
        result.push_back("UF_WORKSTATION_TRUST_ACCOUNT");

    if (flags & UF_SERVER_TRUST_ACCOUNT)
        result.push_back("UF_SERVER_TRUST_ACCOUNT");

    if (flags & UF_DONT_EXPIRE_PASSWD)
        result.push_back("UF_DONT_EXPIRE_PASSWD (Password Never Expires)");

    if (flags & UF_MNS_LOGON_ACCOUNT)
        result.push_back("UF_MNS_LOGON_ACCOUNT");

    if (flags & UF_SMARTCARD_REQUIRED)
        result.push_back("UF_SMARTCARD_REQUIRED");

    if (flags & UF_TRUSTED_FOR_DELEGATION)
        result.push_back("UF_TRUSTED_FOR_DELEGATION");

    if (flags & UF_NOT_DELEGATED)
        result.push_back("UF_NOT_DELEGATED");

    if (flags & UF_USE_DES_KEY_ONLY)
        result.push_back("UF_USE_DES_KEY_ONLY");

    if (flags & UF_DONT_REQUIRE_PREAUTH)
        result.push_back("UF_DONT_REQUIRE_PREAUTH");

    if (flags & UF_PASSWORD_EXPIRED)
        result.push_back("UF_PASSWORD_EXPIRED");

    if (result.empty())
        result.push_back("NO_FLAGS");

    return result;
}


static std::string userPrivilegeToText(DWORD priv)
{
    switch (priv)
    {
    case USER_PRIV_GUEST:
        return "USER_PRIV_GUEST (Guest)";

    case USER_PRIV_USER:
        return "USER_PRIV_USER (Normal User)";

    case USER_PRIV_ADMIN:
        return "USER_PRIV_ADMIN (Administrator)";

    default:
        return "UNKNOWN_PRIVILEGE";
    }
}


static NET_API_STATUS getLocalGroupNameFromSid(WELL_KNOWN_SID_TYPE sidType, LPWSTR* groupName)
{
    *groupName = nullptr;

    DWORD sidSize = SECURITY_MAX_SID_SIZE;

    PSID sid = LocalAlloc(LMEM_FIXED, sidSize);
    if (!sid)
        return ERROR_NOT_ENOUGH_MEMORY;

    if (!CreateWellKnownSid(
        sidType,
        nullptr,
        sid,
        &sidSize))
    {
        DWORD err = GetLastError();
        LocalFree(sid);
        return err;
    }

    WCHAR name[256];
    WCHAR domain[256];

    DWORD nameLen = ARRAYSIZE(name);
    DWORD domainLen = ARRAYSIZE(domain);

    SID_NAME_USE use;

    if (!LookupAccountSidW(
        nullptr,
        sid,
        name,
        &nameLen,
        domain,
        &domainLen,
        &use))
    {
        DWORD err = GetLastError();
        LocalFree(sid);
        return err;
    }

    LocalFree(sid);

    size_t len = wcslen(name) + 1;

    *groupName = (LPWSTR)LocalAlloc(
        LMEM_FIXED,
        len * sizeof(WCHAR));

    if (!*groupName)
        return ERROR_NOT_ENOUGH_MEMORY;

    wcscpy_s(*groupName, len, name);

    return NERR_Success;
}


static NET_API_STATUS addUserToLocalGroup(LPCWSTR username, WELL_KNOWN_SID_TYPE groupSid)
{
    LPWSTR groupName = nullptr;

    NET_API_STATUS status = getLocalGroupNameFromSid(groupSid, &groupName);
    if (status != NERR_Success)
        return status;

    LOCALGROUP_MEMBERS_INFO_3 member = {0};
    member.lgrmi3_domainandname = (LPWSTR)username;

    status = NetLocalGroupAddMembers(
        nullptr,           //for local user
        groupName,
        3,                 //LOCALGROUP_MEMBERS_INFO_3
        (LPBYTE)&member,
        1);

    LocalFree(groupName);

    return status;
}

static NET_API_STATUS removeUserFromLocalGroup(LPCWSTR username, WELL_KNOWN_SID_TYPE groupSid)
{
    LPWSTR groupName = nullptr;

    NET_API_STATUS status = getLocalGroupNameFromSid(groupSid, &groupName);
    if (status != NERR_Success)
        return status;

    LOCALGROUP_MEMBERS_INFO_3 member = { 0 };
    member.lgrmi3_domainandname = (LPWSTR)username;

    status = NetLocalGroupDelMembers(
        nullptr,           //for local user
        groupName,
        3,                 //LOCALGROUP_MEMBERS_INFO_3
        (LPBYTE)&member,
        1);

    LocalFree(groupName);

    return status;
}

void UserManagement::printDomainInfo(char* format) {
    PDOMAIN_CONTROLLER_INFO pDCInfo = nullptr;

    DWORD result = DsGetDcName(
        nullptr,        // ComputerName
        nullptr,        // DomainName (NULL = current domain)
        nullptr,        // DomainGuid
        nullptr,        // SiteName
        DS_DIRECTORY_SERVICE_REQUIRED,
        &pDCInfo);

    if (result == ERROR_SUCCESS)
    {

        if (strcmp(format, "vertical") == 0) {
            // Print Header
            std::cout << "Domain Controller Name~~";
            std::cout << "Domain Controller Address~~";
            std::cout << "Domain Name~~";
            std::cout << "DNS Forest Name~~";
            std::cout << "Client Site~~";
            std::cout << "Flags";
            std::cout << std::endl;

            // Print Body
            std::cout << AppUtil::wideStrToUTF8(pDCInfo->DomainControllerName) << "~~";
            std::cout << AppUtil::wideStrToUTF8(pDCInfo->DomainControllerAddress) << "~~";
            std::cout << AppUtil::wideStrToUTF8(pDCInfo->DomainName) << "~~";
            std::cout << AppUtil::wideStrToUTF8(pDCInfo->DnsForestName) << "~~";
            std::cout << AppUtil::wideStrToUTF8(pDCInfo->ClientSiteName) << "~~";
            std::cout << "0x" << std::hex << pDCInfo->Flags;
            std::cout << std::endl;

        }
        else if (strcmp(format, "horizontal") == 0) {
            std::cout << "Domain Controller Name : " << AppUtil::wideStrToUTF8(pDCInfo->DomainControllerName) << std::endl;
            std::cout << "Domain Controller Address : " << AppUtil::wideStrToUTF8(pDCInfo->DomainControllerAddress) << std::endl;
            std::cout << "Domain Name : " << AppUtil::wideStrToUTF8(pDCInfo->DomainName) << std::endl;
            std::cout << "DNS Forest Name : " << AppUtil::wideStrToUTF8(pDCInfo->DnsForestName) << std::endl;
            std::cout << "Client Site : " << AppUtil::wideStrToUTF8(pDCInfo->ClientSiteName) << std::endl;
            std::cout << "Flags : 0x" << std::hex << pDCInfo->Flags << std::endl;
        }
        NetApiBufferFree(pDCInfo);
    }
    else
    {
        std::cerr << "Error : " << result << ", Desc : " << AppUtil::getErrorDescription(result) << std::endl;
    }
}



void UserManagement::getUserInfo(char* domainName, char* userName, char* format, bool printHeaderInVerticalFormat)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    LPWSTR domainNameW = nullptr; // Local computer
    if (strcmp(domainName, "") != 0) {
        domainNameW = AppUtil::convertUTF8ToLPWSTR(domainName); //"abc.xyz.in"
        if (!domainNameW) {
            std::cerr << "Error : Failed to convert Domainname as wide." << std::endl;
            return;
        }
    }

    if (format == NULL || strcmp(format, "") == 0 ) {
        format = const_cast<char*>("horizontal");
    }

    LPUSER_INFO_4 pInfo = nullptr;

    NET_API_STATUS status = NetUserGetInfo(          
        domainNameW,         //servername - "abc.xyz.in" for Domain User or "nullptr" for Local User
        userNameW,  
        4,                  // USER_INFO_4
        (LPBYTE*)&pInfo);

    if (status == NERR_Success)
    {
        if (strcmp(format, "vertical") == 0) {
           if (printHeaderInVerticalFormat) {
                // Print Header
                std::cout << "Username~~";
                std::cout << "Full Name~~";
                std::cout << "Comment~~";
                std::cout << "Home Dir~~";
                std::cout << "Flags~~";
                std::cout << "Privilege~~";
                std::cout << "Last Logon~~";
                std::cout << "Account Expires~~";
                std::cout << "Bad Pwd Count~~";
                std::cout << "No. of Logons~~";
                std::cout << "User SID~~";
                std::cout << "Password Expired~~";
                std::cout << "Password Age";
                std::cout << std::endl;
           }

            // Print Body
            std::cout << AppUtil::wideStrToUTF8(pInfo->usri4_name) << "~~";
            std::cout << AppUtil::wideStrToUTF8((pInfo->usri4_full_name ? pInfo->usri4_full_name : L"")) << "~~";
            std::cout << AppUtil::wideStrToUTF8((pInfo->usri4_comment ? pInfo->usri4_comment : L"")) << "~~";
            std::cout << AppUtil::wideStrToUTF8((pInfo->usri4_home_dir ? pInfo->usri4_home_dir : L"")) << "~~";

            std::cout << "0x" << std::hex << pInfo->usri4_flags << " [";
            bool firsFlagElem = true;
            for (const auto& flag : userFlagsToText(pInfo->usri4_flags))
            {
                if (firsFlagElem) {
                    std::cout << flag;
                    firsFlagElem = false;
                }
                else {
                    std::cout << ", " << flag;
                }
            }
            std::cout << "]~~";

            std::cout << userPrivilegeToText(pInfo->usri4_priv) << "~~";
            std::cout << AppUtil::convertUTCToLocalTime(pInfo->usri4_last_logon) << "~~";
            std::cout << (pInfo->usri4_acct_expires == TIMEQ_FOREVER ? "Never" : AppUtil::convertUTCToLocalTime(pInfo->usri4_acct_expires)) << "~~";
            std::cout << std::dec << pInfo->usri4_bad_pw_count << "~~";
            std::cout << std::dec << pInfo->usri4_num_logons << "~~";
            std::cout << AppUtil::convertSIDToString(pInfo->usri4_user_sid) << "~~";
            std::cout << passwordExpiredToString(pInfo->usri4_password_expired) << "~~";
            std::cout << AppUtil::convertUTCToLocalTime(pInfo->usri4_password_age);
            std::cout << std::endl;
        }
        else if (strcmp(format, "horizontal") == 0) {
            std::cout << "Username         : " << AppUtil::wideStrToUTF8(pInfo->usri4_name) << std::endl;
            std::cout << "Full Name        : " << AppUtil::wideStrToUTF8((pInfo->usri4_full_name ? pInfo->usri4_full_name : L"")) << std::endl;
            std::cout << "Comment          : " << AppUtil::wideStrToUTF8((pInfo->usri4_comment ? pInfo->usri4_comment : L"")) << std::endl;
            std::cout << "Home Dir         : " << AppUtil::wideStrToUTF8((pInfo->usri4_home_dir ? pInfo->usri4_home_dir : L"")) << std::endl;

            std::cout << "Flags            : 0x" << std::hex << pInfo->usri4_flags << " [";
            bool firsFlagElem = true;
            for (const auto& flag : userFlagsToText(pInfo->usri4_flags))
            {
                if (firsFlagElem) {
                    std::cout << flag;
                    firsFlagElem = false;
                }
                else {
                    std::cout << ", " << flag;
                }
            }
            std::cout << "]" << std::endl;

            std::cout << "Privilege        : " << userPrivilegeToText(pInfo->usri4_priv) << std::endl;
            std::cout << "Last Logon       : " << AppUtil::convertUTCToLocalTime(pInfo->usri4_last_logon) << std::endl;
            std::cout << "Account Expires  : " << (pInfo->usri4_acct_expires == TIMEQ_FOREVER ? "Never" : AppUtil::convertUTCToLocalTime(pInfo->usri4_acct_expires)) << std::endl;
            std::cout << "Bad Pwd Count    : " << std::dec << pInfo->usri4_bad_pw_count << std::endl;
            std::cout << "No. of Logons    : " << std::dec << pInfo->usri4_num_logons << std::endl;
            std::cout << "User SID         : " << AppUtil::convertSIDToString(pInfo->usri4_user_sid) << std::endl;
            std::cout << "Password Expired : " << passwordExpiredToString(pInfo->usri4_password_expired) << std::endl;
            std::cout << "Password Age     : " << AppUtil::convertUTCToLocalTime(pInfo->usri4_password_age) << std::endl;
            std::cout << std::endl;
        }

        NetApiBufferFree(pInfo);
    }
    else
    {
        std::cerr << "Error : " << status << ", Desc : " << AppUtil::getErrorDescription(status) << std::endl;
    }

}



void UserManagement::getAllUsersInfo(char* domainName, char* format)
{
    LPWSTR domainNameW = nullptr; // Local computer
    if (strcmp(domainName, "") != 0) {
        domainNameW = AppUtil::convertUTF8ToLPWSTR(domainName); //"abc.xyz.in"
        if (!domainNameW) {
            std::cerr << "Error : Failed to convert Domainname as wide." << std::endl;
            return;
        }
    }

    LPUSER_INFO_0 pUserInfoBuf = nullptr;
    DWORD entriesRead = 0;
    DWORD totalEntries = 0;
    DWORD resumeHandle = 0;

    NET_API_STATUS status;

    do
    {
        status = NetUserEnum(
            domainNameW,    //servername - "abc.xyz.in" for Domain User or "nullptr" for Local User
            0,              //USER_INFO_0
            FILTER_NORMAL_ACCOUNT,
            (LPBYTE*)&pUserInfoBuf,
            MAX_PREFERRED_LENGTH,
            &entriesRead,
            &totalEntries,
            &resumeHandle);

        if (status == NERR_Success || status == ERROR_MORE_DATA)
        {
            std::cout << "Total Users - "<< entriesRead << std::endl;
            std::cout << "----------------------------------------------" << std::endl;
            boolean headerPrinted = true;
            for (DWORD i = 0; i < entriesRead; i++)
            {
                getUserInfo(domainName, (AppUtil::WCharToUtf8(pUserInfoBuf[i].usri0_name)).data(), format, headerPrinted);
                //std::cout << "" << std::endl;
                if (headerPrinted) {
                    headerPrinted = false;
                }
            }
        }

        if (pUserInfoBuf)
        {
            NetApiBufferFree(pUserInfoBuf);
            pUserInfoBuf = nullptr;
        }

    } while (status == ERROR_MORE_DATA);

}



void UserManagement::verifyUserLogin(char* domainName, char* userName, char* password)
{
    if (domainName == NULL || strcmp(domainName, "") == 0) {
        domainName = const_cast<char*>(".");
    }

    HANDLE hToken = nullptr;
    BOOL userLoggedIn = LogonUserA(
        userName,
        domainName,          // "domain name" OR "." for local machine
        password,
        LOGON32_LOGON_INTERACTIVE,
        LOGON32_PROVIDER_DEFAULT,
        &hToken);

    if (userLoggedIn)
    {
        std::cout << "Login Success, User Credentials are Valid." << std::endl;
        CloseHandle(hToken);
    }
    else
    {
        std::cout << "Login Failed, User Credentials are Invalid." << std::endl;
        DWORD errorCode = GetLastError();
        std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
    }
}



void UserManagement::createLocalUser(char* userName, char* password, char* userAccountComment)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    LPWSTR passwordW = AppUtil::convertUTF8ToLPWSTR(password);
    if (!passwordW) {
        std::cerr << "Error : Failed to convert Password as wide." << std::endl;
        return;
    }

    LPWSTR userAccountCommentW = AppUtil::convertUTF8ToLPWSTR(userAccountComment);
    if (!userAccountCommentW) {
        std::cerr << "Error : Failed to convert UserAccountComment as wide." << std::endl;
        return;
    }


    USER_INFO_1 ui = {};
    ui.usri1_name = userNameW;
    ui.usri1_password = passwordW;
    ui.usri1_priv = USER_PRIV_USER;
    ui.usri1_home_dir = nullptr;
    ui.usri1_comment = userAccountCommentW;
    ui.usri1_flags = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;
    ui.usri1_script_path = nullptr;

    DWORD parmErr = 0;

    NET_API_STATUS status = NetUserAdd(
        nullptr,       //null for creating Local user account
        1,            //USER_INFO_1 
        (LPBYTE)&ui,
        &parmErr);


    switch (status)
    {
        case NERR_Success:
            std::cout << "Local User Account Created Successfully." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cout << "Error: Access denied." << std::endl;
            break;
        case NERR_InvalidComputer:
            std::cout << "Error: The computer name is invalid." << std::endl;
            break;
        case NERR_GroupExists:
            std::cout << "Error: The group already exists." << std::endl;
            break;
        case NERR_UserExists:
            std::cout << "Error : Local User '" << userName << "' already exists." << std::endl;
            break;
        case NERR_PasswordTooShort:
            std::cout << "Error: Password does not meet the system's password policy." << std::endl;
            break;
        case ERROR_INVALID_PASSWORD:
            std::cout << "Error: Invalid Password." << std::endl;
            break;
        default:
            std::cout << "Error : Local User Account Creation Failed. Status - " << status << std::endl;
            DWORD errorCode = GetLastError();
            std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
            break;
    }
}



void UserManagement::addUserToAdministratorGroup(char* userName)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    WELL_KNOWN_SID_TYPE builtInAdministratorSidType = WinBuiltinAdministratorsSid;
    NET_API_STATUS status =  addUserToLocalGroup( userNameW, builtInAdministratorSidType);


    switch (status)
    {
        case NERR_Success:
            std::cout << "User '" << userName << "' is now an Administrator." << std::endl;
            break;
        case ERROR_MEMBER_IN_ALIAS:
            std::cout << "Error : User '" << userName << "' is already an Administrator." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cout << "Error : Access denied." << std::endl;
            break;
        case NERR_GroupNotFound:
            std::cout << "Error : The local group specified by the groupname parameter does not exist." << std::endl;
            break;
        case ERROR_NO_SUCH_MEMBER:
            std::cout << "Error : User '" << userName << "' do not exist." << std::endl;
            break;
        case ERROR_INVALID_MEMBER:
            std::cout << "Error : User '" << userName << "' cannot be added because their account type is invalid." << std::endl;
            break;
        default:
            std::cout << "Error : Failed to add User '" << userName << "' to Administrators. Status - " << status << std::endl;
            DWORD errorCode = GetLastError();
            std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
            break;
    }
}



void UserManagement::removeUserFromAdministratorGroup(char* userName)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    WELL_KNOWN_SID_TYPE builtInAdministratorSidType = WinBuiltinAdministratorsSid;
    NET_API_STATUS status = removeUserFromLocalGroup(userNameW, builtInAdministratorSidType);


    switch (status)
    {
    case NERR_Success:
        std::cout << "User '" << userName << "' removed from Administrators group." << std::endl;
        break;
    case ERROR_MEMBER_IN_ALIAS:
        std::cout << "Error : User '" << userName << "' is not a Administrator." << std::endl;
        break;
    case ERROR_ACCESS_DENIED:
        std::cout << "Error : Access denied." << std::endl;
        break;
    case NERR_GroupNotFound:
        std::cout << "Error : The local group specified by the groupname parameter does not exist." << std::endl;
        break;
    case ERROR_NO_SUCH_MEMBER:
        std::cout << "Error : User '" << userName << "' do not exist." << std::endl;
        break;
    default:
        std::cout << "Error : Failed to remove User '" << userName << "' from Administrators group. Status - " << status << std::endl;
        DWORD errorCode = GetLastError();
        std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
        break;
    }
}



void UserManagement::addUserToStandardNormalUserGroup(char* userName)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    WELL_KNOWN_SID_TYPE builtInNormalUserSidType = WinBuiltinUsersSid;
    NET_API_STATUS status = addUserToLocalGroup(userNameW, builtInNormalUserSidType);


    switch (status)
    {
    case NERR_Success:
        std::cout << "User '" << userName << "' is now an Standard Normal User." << std::endl;
        break;
    case ERROR_MEMBER_IN_ALIAS:
        std::cout << "Error : User '" << userName << "' is already an Standard Normal User." << std::endl;
        break;
    case ERROR_ACCESS_DENIED:
        std::cout << "Error : Access denied." << std::endl;
        break;
    case NERR_GroupNotFound:
        std::cout << "Error : The local group specified by the groupname parameter does not exist." << std::endl;
        break;
    case ERROR_NO_SUCH_MEMBER:
        std::cout << "Error : User '" << userName << "' do not exist." << std::endl;
        break;
    case ERROR_INVALID_MEMBER:
        std::cout << "Error : User '" << userName << "' cannot be added because their account type is invalid." << std::endl;
        break;
    default:
        std::cout << "Error : Failed to add User '" << userName << "' to Standard Normal Users. Status - " << status << std::endl;
        DWORD errorCode = GetLastError();
        std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
        break;
    }
}



void UserManagement::deleteLocalUser(char* userName)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    NET_API_STATUS status = NetUserDel(
        NULL,       // Local computer
        userNameW);


    switch (status)
    {
        case NERR_Success:
            std::cout << "Local User Account Deleted Successfully." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cout << "Error: Access denied." << std::endl;
            break;
        case NERR_InvalidComputer:
            std::cout << "Error: The computer name is invalid." << std::endl;
            break;
        case NERR_UserNotFound:
            std::cout << "Error: User '" << userName << "' does not exist." << std::endl;
            break;
        default:
            std::cout << "Error : Local User Account Deletion Failed. Status - " << status << std::endl;
            DWORD errorCode = GetLastError();
            std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
            break;
    }
}



void UserManagement::setUserPassword(char* userName, char* password)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    LPWSTR passwordW = AppUtil::convertUTF8ToLPWSTR(password);
    if (!passwordW) {
        std::cerr << "Error : Failed to convert Password as wide." << std::endl;
        return;
    }

    USER_INFO_1003 ui = {};
    ui.usri1003_password = passwordW;

    NET_API_STATUS status = NetUserSetInfo(
        NULL,         //null for creating Local user account
        userNameW,
        1003,         //USER_INFO_1003
        (LPBYTE)&ui,
        NULL);

    switch (status)
    {
        case NERR_Success:
            std::cout << "Password Set Successfully for User '" << userName << "'" << std::endl;
            break;
        case NERR_UserNotFound:
            std::cout << "Error: User '" << userName << "' does not exist." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cout << "Error : Access denied." << std::endl;
            break;
        case ERROR_INVALID_PARAMETER:
            std::cout << "Error : Function parameters is invalid." << std::endl;
            break;
        case NERR_InvalidComputer:
            std::cout << "Error : The computer name is invalid." << std::endl;
            break;
        case NERR_LastAdmin:
            std::cout << "Error : The operation is not allowed on the last administrative account." << std::endl;
            break;
        case NERR_BadPassword:
            std::cout << "Error : Password is invalid." << std::endl;
            break;
        case NERR_PasswordTooShort:
            std::cout << "Error : Password does not meet the system's password policy." << std::endl;
            break;
        case ERROR_INVALID_PASSWORD:
            std::cout << "Error : Invalid Password." << std::endl;
            break;
        default:
        {
            std::cout << "Error : Failed to set Password for User '" << userName << "'. Status - " << status << std::endl;
            DWORD errorCode = GetLastError();
            std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
            break;
        }
    }
}



void UserManagement::changeUserPassword(char* userName, char* currentPassword, char* newPassword)
{
    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }

    LPWSTR currentPasswordW = AppUtil::convertUTF8ToLPWSTR(currentPassword);
    if (!currentPasswordW) {
        std::cerr << "Error : Failed to convert OldPassword as wide." << std::endl;
        return;
    }

    LPWSTR newPasswordW = AppUtil::convertUTF8ToLPWSTR(newPassword);
    if (!newPasswordW) {
        std::cerr << "Error : Failed to convert NewPassword as wide." << std::endl;
        return;
    }


    NET_API_STATUS status = NetUserChangePassword(
            NULL,             //null for creating Local user account
            userNameW,
            currentPasswordW,
            newPasswordW);

    switch (status)
    {
        case NERR_Success:
            std::cout << "Password changed Successfully for User '" << userName << "'" << std::endl;
            break;
        case NERR_UserNotFound:
            std::cout << "Error: User '" << userName << "' does not exist." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cout << "Error : Access denied." << std::endl;
            break;
        case ERROR_INVALID_PARAMETER:
            std::cout << "Error : Function parameters is invalid." << std::endl;
            break;
        case NERR_InvalidComputer:
            std::cout << "Error : The computer name is invalid." << std::endl;
            break;
        case NERR_PasswordTooShort:
            std::cout << "Error : Password does not meet the system's password policy." << std::endl;
            break;
        case ERROR_INVALID_PASSWORD:
            std::cout << "Error : Invalid Password." << std::endl;
            break;
        default:
        {
            std::cout << "Error : Failed to change Password for User '" << userName << "'. Status - " << status << std::endl;
            DWORD errorCode = GetLastError();
            std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
            break;
        }
    }
}



void UserManagement::unlockUserAcount(char* domainName, char* userName) {
    LPWSTR domainNameW = nullptr; // Local computer
    if (strcmp(domainName, "") != 0) {
        domainNameW = AppUtil::convertUTF8ToLPWSTR(domainName); //"abc.xyz.in"
        if (!domainNameW) {
            std::cerr << "Error : Failed to convert Domainname as wide." << std::endl;
            return;
        }
    }

    LPWSTR userNameW = AppUtil::convertUTF8ToLPWSTR(userName);
    if (!userNameW) {
        std::cerr << "Error : Failed to convert Username as wide." << std::endl;
        return;
    }


    USER_INFO_4* pInfo = NULL;
    NET_API_STATUS status = NetUserGetInfo(
                domainNameW,
                userNameW,
                4,
                (LPBYTE*)&pInfo);

    if (status != NERR_Success)
    {
        std::cout << "Error : NetUserGetInfo failed: " << status << std::endl;
        return;
    }


    USER_INFO_1008 ui;
    // Clear the UF_LOCKOUT bit
    ui.usri1008_flags = pInfo->usri4_flags & ~UF_LOCKOUT;

    status = NetUserSetInfo(
        domainNameW,
        userNameW,
        1008,
        (LPBYTE)&ui,
        NULL);

    switch (status)
    {
        case NERR_Success:
            std::cout << "Account Unlocked Successfully for User '" << userName << "'" << std::endl;
            break;
        case NERR_UserNotFound:
            std::cout << "Error: User '" << userName << "' does not exist." << std::endl;
            break;
        case ERROR_ACCESS_DENIED:
            std::cout << "Error : Access denied." << std::endl;
            break;
        case ERROR_INVALID_PARAMETER:
            std::cout << "Error : Function parameters is invalid." << std::endl;
            break;
        case NERR_InvalidComputer:
            std::cout << "Error : The computer name is invalid." << std::endl;
            break;
        case NERR_PasswordTooShort:
            std::cout << "Error : Password does not meet the system's password policy." << std::endl;
            break;
        case NERR_LastAdmin:
            std::cout << "Error : Operation is not allowed on the last administrative account.." << std::endl;
            break;
        default:
        {
            std::cout << "Error : Failed to unlock Account for User '" << userName << "'. Status - " << status << std::endl;
            DWORD errorCode = GetLastError();
            std::cerr << "Error : " << errorCode << ", Desc : " << AppUtil::getErrorDescription(errorCode) << std::endl;
            break;
        }
    }

    NetApiBufferFree(pInfo);
}





