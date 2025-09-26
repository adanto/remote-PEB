# remote-PEB

Small PoC to enumerate local processes and read each process' PEB (via `NtQueryInformationProcess` + `ReadProcessMemory`) to display command line, image path and a few PEB fields.

Uses community NT-internals headers (PHNT / systeminformer-style) so the PoC can rely on correct `PEB` / `RTL_USER_PROCESS_PARAMETERS` / `PROCESS_BASIC_INFORMATION` types.

---

## Features

- Enumerates processes (Toolhelp snapshot)
- Reads remote PEB and `RTL_USER_PROCESS_PARAMETERS`
- Prints: `CommandLine`, `ImagePathName`, `BeingDebugged`, `ImageBaseAddress`, OS version fields
- Lightweight, single-purpose PoC — useful for research / triage

---

## Requirements

- Windows 10/11 (x64 recommended)
- Tested in Visual Studio 2022
- `external/phnt` headers (included as a submodule)

---

## Build

**Visual Studio**

* Open `build/remote-PEB.sln`
* Select `Release | x64` and build

---

## Usage

Run from an elevated prompt to maximize the number of readable processes:

```powershell
.\remote-PEB.exe
```

---

## Example output (sanitized)

> This example is redacted to avoid leaking usernames, hostnames or absolute paths.

```
.\remote-PEB.exe

PID 12212:
         CommandLine: "C:\Program Files\<Vendor>\<App>\app.exe" -f "<redacted-log>" -p 30000
       ImagePathName: C:\Program Files\<Vendor>\<App>\app.exe
       BeingDebugged: 0
    ImageBaseAddress: 0xE32F0000
      OSMajorVersion: 10
      OSMinorVersion: 0
       OSBuildNumber: 26100
        OSCSDVersion: 0

PID 12984:
         CommandLine: sihost.exe
       ImagePathName: C:\Windows\System32\sihost.exe
       BeingDebugged: 0
    ImageBaseAddress: 0xAAAAFFFF
      OSMajorVersion: 10
      OSMinorVersion: 0
       OSBuildNumber: 26100
        OSCSDVersion: 0

PID 11872:
         CommandLine: C:\Windows\System32\svchost.exe -k UnistackSvcGroup -s <ServiceName>
       ImagePathName: C:\Windows\System32\svchost.exe
       BeingDebugged: 0
    ImageBaseAddress: 0xDEADBEEF
      OSMajorVersion: 10
      OSMinorVersion: 0
       OSBuildNumber: 26100
        OSCSDVersion: 0
```

---

## Notes & caveats

* The PoC currently targets same-bitness reading. For cross-bitness (x86 ↔ x64) add WOW64 handling (`IsWow64Process2` / 32-bit PEB structs).
* Protected/PPL processes will refuse access; expect `ERROR_ACCESS_DENIED`.
* Prefer `PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ` over `PROCESS_ALL_ACCESS`. Enabling `SeDebugPrivilege` may be required for some processes (helper included under `tools/`).
