
#include "apputil.h"



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


	//Get Windows Error Description
	std::string getErrorDescription(DWORD errorCode)
	{
		char* buffer = nullptr;

		DWORD length = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			errorCode,
			0,
			(LPSTR)&buffer,
			0,
			nullptr);

		std::string message;

		if (length && buffer)
		{
			message.assign(buffer, length);

			// Remove trailing newline characters
			while (!message.empty() &&
				(message.back() == '\n' || message.back() == '\r'))
			{
				message.pop_back();
			}

			LocalFree(buffer);
		}
		else
		{
			message = "Unknown error";
		}

		return message;
	}


	//Windows timestamp in seconds since January 1, 1970 (UTC) (Unix-style time).
	std::string convertUTCToLocalTime(DWORD lastLogon)
	{
		if (lastLogon == 0)
			return "Never";

		std::time_t t = static_cast<std::time_t>(lastLogon);
		std::tm localTime{};
		localtime_s(&localTime, &t);

		char buffer[64];
		std::strftime(buffer, sizeof(buffer), "%d-%b-%Y %H:%M:%S", &localTime);
		return buffer;
	}


	//Converts SID to String
	std::string convertSIDToString(PSID sid)
	{
		if (sid == nullptr)
			return "";

		LPSTR sidString = nullptr;

		if (!ConvertSidToStringSidA(sid, &sidString))
			return "";

		std::string result(sidString);

		LocalFree(sidString);

		return result;
	}


	//Returns the Commandline of the given process handle
	std::wstring getProcessCommandLine(HANDLE hProcess)
	{
		std::wstring result;

		if (!hProcess)
			return result;

		HMODULE hNtDll =
			GetModuleHandleW(L"ntdll.dll");

		if (!hNtDll)
			return result;

		auto NtQueryInformationProcess =
			(_NtQueryInformationProcess)GetProcAddress(
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


	//Returns the Full Path of the given process handle
	std::wstring getProcessPath(HANDLE hProcess)
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


	//Returns the user name of the given process handle with which session the process is running
	std::wstring getProcessUser(HANDLE hProcess)
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

}



