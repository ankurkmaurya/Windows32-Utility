#pragma once

#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>


namespace AppUtil {

	LPWSTR convertUTF8ToLPWSTR(char* utf8chars);

	std::string convertBSTRToString(BSTR bstr);

	bool isFileExists(const std::string& filePath);

	std::string wideStrToUTF8(const std::wstring& wstr);

	std::string WCharToUtf8(const wchar_t* wstr);
}