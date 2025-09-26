#include "remote-PEB.h"

int LoadNtQueryAndGetPEB(HANDLE hProcess);
int IterateOverProcesses();
void debugPrintPEBInfo(PPEB pPEB, WCHAR* ImagePathNameBuffer, WCHAR* CommandLineBuffer);

int main()
{
	IterateOverProcesses();
	return 0;
}

int IterateOverProcesses() {

	HANDLE hSnapshot;
	PROCESSENTRY32 pe;
	BOOL hResult;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) { return 0; }

	pe.dwSize = sizeof(PROCESSENTRY32);

	hResult = Process32First(hSnapshot, &pe);

	// Iterate over all processes in the snapshot (we cannot access all of them)
	while (hResult) {

		if (HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pe.th32ProcessID))
		{
			printf("PID %i: ", pe.th32ProcessID);

			// Pass the handle to read the memory from the process and access the PEB 
			LoadNtQueryAndGetPEB(hProcess);

			CloseHandle(hProcess);
		}
		hResult = Process32Next(hSnapshot, &pe);

	}

	CloseHandle(hSnapshot);

	return 1;
}

// Load NtQueryInformationProcess function and get PEB information for the given process
int LoadNtQueryAndGetPEB(HANDLE hProcess)
{
	// The idea of using the undocumented function comes from espresso3389 (https://espresso3389.hatenablog.com/entries/2008/07/23)
	// Load ntdll.dll 
	HINSTANCE hNtDll = GetModuleHandleW(L"ntdll.dll");
	// Get the address of the NtQueryInformationProcess function
	NtQueryInformationProcessPtr NtQueryInformationProcess = (NtQueryInformationProcessPtr)GetProcAddress(hNtDll, "NtQueryInformationProcess");
	// Get the address of the RtlNtStatusToDosError function in case we need it 
	RtlNtStatusToDosErrorPtr RtlNtStatusToDosError = (RtlNtStatusToDosErrorPtr)GetProcAddress(hNtDll, "RtlNtStatusToDosError");

	// Check if the functions were loaded successfully
	if (!NtQueryInformationProcess || !RtlNtStatusToDosError)
	{
		fprintf(stderr, "\n\t[!] Could not load module or functions from ntdll.dll\n");
		return -1;
	}

	PROCESS_BASIC_INFORMATION pbi;
	ULONG len;

	// Query process information to get the PEB address
	NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &len);
	SetLastError(RtlNtStatusToDosError(status));
	if (NT_ERROR(status) || !pbi.PebBaseAddress)
	{
		fprintf(stderr, "\n\t[!] API call NtQueryInformationProcess failed\n");
		return -1;
	}

	// Buffer to read the PEB structure
	void* PEBbuffer[sizeof(PEB)];
	SIZE_T bytesRead = 0;

	// Read the PEB structure from the target process
	if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, PEBbuffer, sizeof(PEB), &bytesRead)) {
		fprintf(stderr, "\n\t[!] Could not read PEB base\n");
		return -1;
	}

	PPEB pPEB = reinterpret_cast<PPEB>(PEBbuffer);

	// Buffer to read the process parameters structure
	LPVOID bufferPParams[sizeof(RTL_USER_PROCESS_PARAMETERS)];

	// Read the RTL_USER_PROCESS_PARAMETERS structure (containing cmdline and path) from the target process
	if (!ReadProcessMemory(hProcess, (LPCVOID)(pPEB->ProcessParameters), bufferPParams, sizeof(RTL_USER_PROCESS_PARAMETERS), &bytesRead))
	{
		fprintf(stderr, "\n\t[!] Could not read memory from PROCESS PARAMETERS\n");
		return -1;
	};

	PRTL_USER_PROCESS_PARAMETERS pProcParams = reinterpret_cast<PRTL_USER_PROCESS_PARAMETERS>(bufferPParams);

	// Buffer to read the command line
	WCHAR* CommandLineBuffer = new WCHAR[(pProcParams->CommandLine.Length / sizeof(wchar_t)) + 0x10];
	CommandLineBuffer[(pProcParams->CommandLine.Length / sizeof(wchar_t)) + 1] = L'\0';

	// Buffer to read the image path
	WCHAR ImagePathNameBuffer[MAX_PATH];
	ImagePathNameBuffer[(pProcParams->ImagePathName.Length / sizeof(wchar_t)) + 1] = L'\0';

	// Read the command line from the process memory
	if (!ReadProcessMemory(hProcess, pProcParams->CommandLine.Buffer, CommandLineBuffer, pProcParams->CommandLine.MaximumLength, &bytesRead))
	{
		fprintf(stderr, "\n\t[!] Could not read memory from pProcParams->CommandLine.Buffer\n");
		return -1;
	};

	// Read the image path name from the process memory
	if (!ReadProcessMemory(hProcess, pProcParams->ImagePathName.Buffer, ImagePathNameBuffer, pProcParams->ImagePathName.MaximumLength, &bytesRead))
	{
		fprintf(stderr, "\n\t[!] Could not read memory from pProcParams->ImagePathName.Buffer\n");
		return -1;
	};

	// Print some debug information from the PDB 
	debugPrintPEBInfo(pPEB, ImagePathNameBuffer, CommandLineBuffer);

	return true;

}

void debugPrintPEBInfo(PPEB pPEB, WCHAR* ImagePathNameBuffer, WCHAR* CommandLineBuffer)
{
	wprintf(L"\n%20s: %s", L"CommandLine", CommandLineBuffer);
	wprintf(L"\n%20s: %s", L"ImagePathName", ImagePathNameBuffer);
	printf("\n%20s: %d", "BeingDebugged", pPEB->BeingDebugged);
	printf("\n%20s: 0x%08x", "ImageBaseAddress", pPEB->ImageBaseAddress);
	printf("\n%20s: %i", "OSMajorVersion", pPEB->OSMajorVersion);
	printf("\n%20s: %i", "OSMinorVersion", pPEB->OSMinorVersion);
	printf("\n%20s: %i", "OSBuildNumber", pPEB->OSBuildNumber);
	printf("\n%20s: %i", "OSCSDVersion", pPEB->OSCSDVersion);

	printf("\n\n");
}