
#include <netact.h>
#include <cstring>
#include <wmiquery.h>
#include <telnet.h>
#include <ping.h>
#include <servicescm.h>
#include <usersession.h>


int main(int argc, char* argv[])
{

    //std::cout << "Argument count: " << argc << "\n";
    char* command = NULL;
    char* option = NULL;

    char* queryKey = NULL;
    char* queryValue = NULL;
    char* queryFormat = NULL;

    char* ipAddress = NULL;
    char* port = NULL;

    char* serviceName = NULL;
    char* serviceDisplayName = NULL;
    char* serviceDescription = NULL;
    char* serviceBinaryPath = NULL;
    char* serviceStartType = NULL;

    char* programFilePath = NULL;
    char* programCommandLine = NULL;
    char* windowMode = NULL;

    char* pId = NULL;


    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-help") == 0)
        {
            std::cout << "Usage Guide:" << std::endl;
            std::cout << "Win32Util [command] [options]" << std::endl;
            std::cout << "Note : Argument containing spaces must be quoted." << std::endl;
            std::cout << std::endl;

            std::cout << "Commands Available - " << std::endl;
            std::cout << "1. wmi          : Provides system information." << std::endl;
            std::cout << "2. telnet       : Provides functionality to test connectivity with IP and Port." << std::endl;
            std::cout << "3. ping         : Provides functionality to test connectivity with host." << std::endl;
            std::cout << "4. service      : Provides managing window services through Service Control Manager (SCM)." << std::endl;
            std::cout << "5. netact       : Provides network activity details of the system with Process information." << std::endl;
            std::cout << "6. usrsesn      : Provides functionality to get system current user session information and start process with specific user session." << std::endl;
            std::cout << "7. -help        : Provides usage guide." << std::endl;
            std::cout << "8. -version     : Displays application version information." << std::endl;
            std::cout << std::endl;

            std::cout << "Command Options - " << std::endl;
            std::cout << "1. wmi command options" << std::endl;
            std::cout << "    cpu key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    logical.disk key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    disk.drive key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    disk.partition key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    physical.memory key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    computer.system key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    operating.system key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    os.hot.fixes key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    network.interface key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    bios key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    system.users key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    process key=all/xxxx value=xxxx format=vertical/horizontal" << std::endl;
            std::cout << std::endl;

            std::cout << "2. telnet command options" << std::endl;
            std::cout << "    ip=xxx.xxx.xxx.xxx" << std::endl;
            std::cout << "    port=xxxxx" << std::endl;
            std::cout << std::endl;

            std::cout << "3. ping command option" << std::endl;
            std::cout << "    ip=xxx.xxx.xxx.xxx" << std::endl;
            std::cout << std::endl;

            std::cout << "4. service command option" << std::endl;
            std::cout << "   [Argument containing spaces must be quoted]" << std::endl;
            std::cout << "    service.query.all format=vertical/horizontal" << std::endl;
            std::cout << "    service.query name=xxxx format=vertical/horizontal" << std::endl;
            std::cout << "    service.install name=xxxx displayname=\"xxxxxxxxxx\" description=\"xxxxx xxxx xxxx\" binarypath=\"xxxxx\\xxxx\\xxx.exe\"" << std::endl;
            std::cout << "    service.uninstall name=xxxx" << std::endl;
            std::cout << "    service.start name=xxxx" << std::endl;
            std::cout << "    service.stop name=xxxx" << std::endl;
            std::cout << "    service.startup.type.change name=xxxx type=auto/manual/disable" << std::endl;
            std::cout << "    service.description name=xxxx" << std::endl;
            std::cout << std::endl;

            std::cout << "5. netact command options" << std::endl;
            std::cout << "    tcp key=all/remoteip/localip/remoteport/localport value=xxxx format=vertical/horizontal/tabular" << std::endl;
            std::cout << std::endl;

            std::cout << "6. usrsesn command options" << std::endl;
            std::cout << "   [Argument containing spaces must be quoted]" << std::endl;
            std::cout << "    usersession.query.info  format=vertical/horizontal/tabular" << std::endl;
            std::cout << "    usersession.active.program.launch  windowmode=default/hidden/minimized  programpath=\"xxxxx\\xxxx\\xxx.exe\"  commandline=xxxxx xxx xxxx" << std::endl;
            std::cout << "    usersession.enumeratewindows  key=all/pid/sessionid/windowstate  value=xxxx  format=vertical/horizontal" << std::endl;
            std::cout << "      -> key:windowstate  value:foreground/minimized/visible/hidden" << std::endl;
            std::cout << "    usersession.process.bring.foreground  pid=xxxx" << std::endl;
            std::cout << std::endl;

            return 0;
        } 
        else if (strcmp(argv[i], "-version") == 0)
        {
            std::cout << "Version - 1.4.0, Published - 16 Jun 2026" << std::endl;
            return 0;
        }
        else if (strcmp(argv[i], "wmi") == 0 || 
                 strcmp(argv[i], "telnet") == 0 ||
                 strcmp(argv[i], "ping") == 0 || 
                 strcmp(argv[i], "service") == 0 ||
                 strcmp(argv[i], "netact") == 0 ||
                 strcmp(argv[i], "usrsesn") == 0)
        {
            command = argv[i];
        }
        else if (strcmp(argv[i], "cpu") == 0 || 
            strcmp(argv[i], "logical.disk") == 0 || 
            strcmp(argv[i], "disk.drive") == 0 || 
            strcmp(argv[i], "disk.partition") == 0 || 
            strcmp(argv[i], "physical.memory") == 0 || 
            strcmp(argv[i], "computer.system") == 0 || 
            strcmp(argv[i], "operating.system") == 0 || 
            strcmp(argv[i], "os.hot.fixes") == 0 || 
            strcmp(argv[i], "network.interface") == 0 || 
            strcmp(argv[i], "bios") == 0 || 
            strcmp(argv[i], "system.users") == 0 || 
            strcmp(argv[i], "process") == 0)
        {
            option = argv[i];
        }
        else if (strncmp(argv[i], "ip=", 3) == 0)
        {    
            option = argv[i];
            ipAddress = argv[i] + 3;
        }
        else if (strncmp(argv[i], "port=", 5) == 0)
        {
            option = argv[i];
            port = argv[i] + 5;
        }
        else if (strncmp(argv[i], "serv=", 5) == 0)
        {
            option = argv[i];
        }
        else if (strcmp(argv[i], "service.query.all") == 0 || 
                 strcmp(argv[i], "service.query") == 0 || 
                 strcmp(argv[i], "service.install") == 0 || 
                 strcmp(argv[i], "service.uninstall") == 0 || 
                 strcmp(argv[i], "service.start") == 0 || 
                 strcmp(argv[i], "service.stop") == 0 || 
                 strcmp(argv[i], "service.startup.type.change") == 0 ||
                 strcmp(argv[i], "service.description") == 0)
        {
            option = argv[i];
        }
        else if (strncmp(argv[i], "key=", 4) == 0)
        {
            queryKey = argv[i] + 4;
        }
        else if (strncmp(argv[i], "value=", 6) == 0)
        {
            queryValue = argv[i] + 6;
        }
        else if (strncmp(argv[i], "format=", 7) == 0)
        {
            queryFormat = argv[i] + 7;
        }
        else if (strncmp(argv[i], "name=", 5) == 0)
        {
            serviceName = argv[i] + 5;
        }
        else if (strncmp(argv[i], "displayname=", 12) == 0)
        {
            serviceDisplayName = argv[i] + 12;
        }
        else if (strncmp(argv[i], "description=", 12) == 0)
        {
            serviceDescription = argv[i] + 12;
        }
        else if (strncmp(argv[i], "binarypath=", 11) == 0)
        {
            serviceBinaryPath = argv[i] + 11;
        }
        else if (strncmp(argv[i], "type=", 5) == 0)
        {
            serviceStartType = argv[i] + 5;
        }
        else if (strcmp(argv[i], "tcp") == 0)
        {
            option = argv[i];
        }
        else if (strcmp(argv[i], "usersession.query.info") == 0)
        {
            option = argv[i];
        }
        else if (strcmp(argv[i], "usersession.active.program.launch") == 0)
        {
            option = argv[i];
        }
        else if (strcmp(argv[i], "usersession.enumeratewindows") == 0)
        {
            option = argv[i];
        }
        else if (strcmp(argv[i], "usersession.process.bring.foreground") == 0)
        {
            option = argv[i];
        }
        else if (strncmp(argv[i], "programpath=", 12) == 0)
        {
            programFilePath = argv[i] + 12;
        }
        else if (strncmp(argv[i], "commandline=", 12) == 0)
        {
            programCommandLine = argv[i] + 12;
        }
        else if (strncmp(argv[i], "windowmode=", 11) == 0)
        {
            windowMode = argv[i] + 11;
        }
        else if (strncmp(argv[i], "pid=", 4) == 0)
        {
            pId = argv[i] + 4;
        }

        //std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
    }

    if (command == NULL) {
        std::cout << "Error : Command not found, Please provide valid Command, Use -help for available Commands" << std::endl;
        return 1;

    }
    //std::cout << "Command - " << command << std::endl;

    if (option == NULL) {
        std::cout << "Error : Option not found, Please provide valid Option, Use -help for available Options" << std::endl;
        return 1;
    }
    //std::cout << "Option - " << option << std::endl;


    if (strcmp(command, "wmi") == 0) {
        WMIQuery wmi;

        if (queryKey == NULL) {
            std::cout << "Error : key parameter is required for wmi '" << option << "' command." << std::endl;
            return 1;
        }
        else if (queryValue == NULL) {
            std::cout << "Error : value parameter is required for wmi '" << option << "' command." << std::endl;
            return 1;
        }
        else if (queryFormat == NULL) {
            std::cout << "Error : format parameter is required for wmi '" << option << "' command." << std::endl;
            return 1;
        }

        std::string wmiQuery = wmi.getWMIQueryFromOption(option, queryKey, queryValue);
        wmi.printWMIQueryResults(wmiQuery, queryFormat);
    } 
    else if (strcmp(command, "telnet") == 0) {
        if (ipAddress == NULL || port == NULL) {
            std::cout << "Error : ip and port parameter is required for telnet command." << std::endl;
            return 1;
        }

        Telnet telnet;
        telnet.checkIPAndPortConnectivity(ipAddress, port);
    }
    else if (strcmp(command, "ping") == 0) {
        if (ipAddress == NULL) {
            std::cout << "Error : ip parameter is required for ping command." << std::endl;
            return 1;
        }

        Ping ping;
        ping.checkIPConnectivity(ipAddress);
    }
    else if (strcmp(command, "service") == 0) {
        ServicesSCM servicesSCM;
        if (strcmp(option, "service.query.all") == 0) {
            if (queryFormat == NULL) {
                std::cout << "Error : format parameter is required for '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.printAllServiceDetails(queryFormat);
        } 
        else if (strcmp(option, "service.query") == 0) {
            if (serviceName == NULL || queryFormat == NULL) {
                std::cout << "Error : name and format parameter is required for '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.printServiceDetails(serviceName, queryFormat);
        }
        else if (strcmp(option, "service.install") == 0) {
            if (serviceName == NULL) {
                std::cout << "Error : name parameter is required for '" << option << "' command." << std::endl;
                return 1;
            } else if (serviceDisplayName == NULL) {
                std::cout << "Error : displayname parameter is required for '" << option << "' command." << std::endl;
                return 1;
            } else if (serviceDescription == NULL) {
                std::cout << "Error : description parameter is required for '" << option << "' command." << std::endl;
                return 1;
            } else if (serviceBinaryPath == NULL) {
                std::cout << "Error : binarypath parameter is required for '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.serviceInstall(serviceName, serviceDisplayName, serviceDescription, serviceBinaryPath);
        }
        else if (strcmp(option, "service.uninstall") == 0) {
            if (serviceName == NULL) {
                std::cout << "Error : name parameter is required for '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.serviceUnInstall(serviceName);
        }
        else if (strcmp(option, "service.start") == 0) {
            if (serviceName == NULL) {
                std::cout << "Error : name parameter is required for '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.serviceStart(serviceName);
        }
        else if (strcmp(option, "service.stop") == 0) {
            if (serviceName == NULL) {
                std::cout << "Error : name parameter is required for '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.serviceStop(serviceName);
        }
        else if (strcmp(option, "service.startup.type.change") == 0) {
            if (serviceName == NULL || serviceStartType == NULL) {
                std::cout << "Error : name and type parameter is required for '" << option << "' command." << std::endl;
                return 1;
            } 
            servicesSCM.serviceStartupTypeChange(serviceName, serviceStartType);
        }
        else if (strcmp(option, "service.description") == 0) {
            if (serviceName == NULL) {
                std::cout << "Error : name parameter is required for  '" << option << "' command." << std::endl;
                return 1;
            }
            servicesSCM.printServiceDescription(serviceName);
        }
    }
    else if (strcmp(command, "netact") == 0) {
        if (queryKey == NULL) {
            std::cout << "Error : key parameter is required for tcpnetact '" << option << "' command." << std::endl;
            return 1;
        }
        else if (queryValue == NULL) {
            std::cout << "Error : value parameter is required for tcpnetact '" << option << "' command." << std::endl;
            return 1;
        }
        else if (queryFormat == NULL) {
            std::cout << "Error : format parameter is required for tcpnetact '" << option << "' command." << std::endl;
            return 1;
        }

        NetworkActivity networkActivity;
        networkActivity.printTCPNetworkActivity(queryKey, queryValue, queryFormat);
    }
    else if (strcmp(command, "usrsesn") == 0) {
        UserSession userSession;
        if (strcmp(option, "usersession.query.info") == 0) {
            if (queryFormat == NULL) {
                std::cout << "Error : format parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }
            userSession.printUserSessionDetails(queryFormat);
        }
        else if (strcmp(option, "usersession.active.program.launch") == 0) {
            if (programFilePath == NULL) {
                std::cout << "Error : programpath parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }
            else if (programCommandLine == NULL) {
                std::cout << "Error : commandline parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }
            else if (windowMode == NULL) {
                std::cout << "Error : windowmode parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }

            WindowMode winMode = userSession.parseWindowMode(windowMode);
            userSession.launchProgramInActiveUserSession(programFilePath, programCommandLine, winMode);
        }
        else if (strcmp(option, "usersession.enumeratewindows") == 0) {
            if (queryKey == NULL) {
                std::cout << "Error : key parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }
            else if (queryValue == NULL) {
                std::cout << "Error : value parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }
            else if (queryFormat == NULL) {
                std::cout << "Error : format parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }

            EnumerateWindowContext context = { 0 };
            context.format = queryFormat;
            context.printHeader = true;
            context.key = queryKey;
            context.value = queryValue;

            userSession.enumerateWindows(context);
        }
        else if (strcmp(option, "usersession.process.bring.foreground") == 0) {
            if (pId == NULL) {
                std::cout << "Error : pId parameter is required for usrsesn '" << option << "' command." << std::endl;
                return 1;
            }
            userSession.bringProcessToForeground(pId);
        }
    }



    return 0;
    
}








