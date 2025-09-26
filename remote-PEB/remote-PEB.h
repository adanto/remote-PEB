#define PHNT_VERSION PHNT_WIN11 // Windows 11

#include <phnt_windows.h>
#include <phnt.h>

#include <tlhelp32.h>

#include <locale.h>
#include <stdio.h>

// https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess
typedef NTSTATUS(NTAPI* NtQueryInformationProcessPtr)(
	HANDLE           ProcessHandle,
	PROCESSINFOCLASS ProcessInformationClass,
	PVOID            ProcessInformation,
	ULONG            ProcessInformationLength,
	PULONG           ReturnLength
	);

// https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-rtlntstatustodoserror
typedef ULONG(NTAPI* RtlNtStatusToDosErrorPtr)(
	NTSTATUS Status
	);