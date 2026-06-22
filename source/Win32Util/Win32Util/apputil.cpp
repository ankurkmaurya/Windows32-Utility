
#include "apputil.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

#include <windows.h>


namespace AppUtil {


	LPWSTR convertUTF8ToLPWSTR(char* utf8chars) {
		if (!utf8chars) return nullptr;

		// Get required buffer size
		int sizeNeeded = MultiByteToWideChar(
			CP_UTF8,            // or CP_ACP for ANSI
			0,
			utf8chars,
			-1,
			nullptr,
			0
		);
		if (sizeNeeded == 0) return nullptr;

		// Allocate buffer (caller must free this later using delete[])
		LPWSTR result = new WCHAR[sizeNeeded];

		// Perform conversion
		MultiByteToWideChar(
			CP_UTF8,
			0,
			utf8chars,
			-1,
			result,
			sizeNeeded
		);
		return result;
	}


	std::string convertBSTRToString(BSTR bstr) {
		if (!bstr) return {};

		int len = WideCharToMultiByte(
			CP_UTF8, 0,
			bstr, SysStringLen(bstr),
			nullptr, 0,
			nullptr, nullptr
		);

		std::string result(len, 0);

		WideCharToMultiByte(
			CP_UTF8, 0,
			bstr, SysStringLen(bstr),
			&result[0], len,
			nullptr, nullptr
		);

		return result;
	}


	bool isFileExists(const std::string& filePath) {
		return std::filesystem::exists(filePath);
	}


	//Convert wide string to UTF-8
	std::string wideStrToUTF8(const std::wstring& wstr) {
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
			nullptr, 0, nullptr, nullptr);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
		return strTo;
	}


	//Convert wide char to UTF-8
	std::string WCharToUtf8(const wchar_t* wstr)
	{
		if (!wstr) {
			return "";
		}
		
		int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		std::string result(size - 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], size, NULL, NULL);
		return result;
	}



}





