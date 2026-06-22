
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

#pragma once


class WMIQuery
{

public:

    /*
     *
      Options                -> option
      CPU                    -> cpu
      Logical Disk           -> logical.disk
      Disk Drive             -> disk.drive
      Disk Partition         -> disk.partition
      Physical Memory        -> physical.memory
      Computer System        -> computer.system
      Operating System (OS)  -> operating.system
      HotFixes               -> os.hot.fixes
      NetworkInterface       -> network.interface
      BIOS                   -> bios
      SystemUsers            -> system.users
      Process                -> process
    *
    */
    std::string getWMIQueryFromOption(char* option, char* key, char* value);

	void printWMIQueryResults(std::string wmiQuery, char* format);

    std::string readVTArrayData(VARIANT vtProp);

    void printQueryDetailsHorizontally(IWbemClassObject* pclsObj, SAFEARRAY* pNames);

    void printQueryDetailsVertically(IWbemClassObject* pclsObj, SAFEARRAY* pNames, BOOL headerPrinted);
};



