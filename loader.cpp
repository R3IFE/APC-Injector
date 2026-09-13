#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

#define REG_KEY_PATH L"SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\Cache"

struct InjectionDiag {
    ULONG64 DllBase;
    ULONG64 ShellcodeAddr;
    ULONG64 LoadLibAddr;
    ULONG64 GetProcAddr;
    ULONG64 RtlAddFuncTableAddr;
    ULONG64 DataPageAddr;
    LONG    Status;
    LONG    ProtectStatus;
    ULONG   ImageSize;
    ULONG   EntryPointRva;
    ULONG   MissingModCount;
    ULONG   UnresolvedCount;
    ULONG   TotalThreads;
    ULONG   ApcCount;
    ULONG   ShellcodeSize;
    ULONG   ExceptionEntries;
    UCHAR   ProgressMarker;
    UCHAR   ProcessAlive;
    UCHAR   pad[6];
    char    MissingMod[8][64];
};

static DWORD FindProcessByName(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* targetName = L"notepad.exe";
    const wchar_t* dllPath = nullptr;

    if (argc >= 2) dllPath = argv[1];
    if (argc >= 3) targetName = argv[2];

    if (!dllPath) {
        std::wcerr << L"Usage: loader.exe <dll_path> [process_name]\n"
                    << L"  dll_path:      full path to DLL (e.g. C:\\Users\\you\\Desktop\\test.dll)\n"
                    << L"  process_name:  target process (default: notepad.exe)\n";
        return 1;
    }

    DWORD pid = FindProcessByName(targetName);
    if (!pid) {
        std::wcerr << L"Process not found: " << targetName << L"\n";
        return 1;
    }
    std::wcout << L"Found " << targetName << L" (PID " << pid << L")\n";

    WCHAR ntPath[300];
    wsprintfW(ntPath, L"\\??\\%s", dllPath);

    HKEY hKey;
    DWORD disp;
    LONG err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, REG_KEY_PATH, 0, nullptr,
                                REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, &disp);
    if (err != ERROR_SUCCESS) {
        std::cerr << "RegCreateKeyEx failed: " << err << " (run as admin)\n";
        return 1;
    }

    RegDeleteValueW(hKey, L"Status");
    RegDeleteValueW(hKey, L"DiagData");

    RegSetValueExW(hKey, L"SessionId", 0, REG_DWORD, reinterpret_cast<BYTE*>(&pid), sizeof(pid));
    RegSetValueExW(hKey, L"DataPath", 0, REG_SZ, reinterpret_cast<const BYTE*>(ntPath),
                   static_cast<DWORD>((wcslen(ntPath) + 1) * sizeof(WCHAR)));

    std::cout << "Request written. Waiting for driver";

    for (int i = 0; i < 120; i++) {
        Sleep(500);
        DWORD statusVal;
        DWORD size = sizeof(statusVal);
        DWORD type;
        if (RegQueryValueExW(hKey, L"Status", nullptr, &type,
                              reinterpret_cast<BYTE*>(&statusVal), &size) == ERROR_SUCCESS) {
            std::cout << "\n";

            LONG ntstatus = static_cast<LONG>(statusVal);
            if (ntstatus < 0) {
                std::cerr << "Injection failed: NTSTATUS 0x"
                          << std::hex << statusVal << "\n";
            } else {
                std::cout << "Injection succeeded: NTSTATUS 0x"
                          << std::hex << statusVal << "\n";
            }

            InjectionDiag diag = {};
            DWORD diagSize = sizeof(diag);
            if (RegQueryValueExW(hKey, L"DiagData", nullptr, nullptr,
                                  reinterpret_cast<BYTE*>(&diag), &diagSize) == ERROR_SUCCESS) {
                std::cout << "=== DIAGNOSTICS ===\n";
                std::cout << "  DLL mapped at:    0x" << std::hex << diag.DllBase << "\n";
                std::cout << "  EntryPoint RVA:   0x" << std::hex << diag.EntryPointRva << "\n";
                std::cout << "  EntryPoint addr:  0x" << std::hex << (diag.DllBase + diag.EntryPointRva) << "\n";
                std::cout << "  ImageSize:        0x" << std::hex << diag.ImageSize << "\n";
                std::cout << "  Shellcode at:     0x" << std::hex << diag.ShellcodeAddr << "\n";
                std::cout << "  Shellcode size:   " << std::dec << diag.ShellcodeSize << " bytes\n";
                std::cout << "  VirtualProtect:   0x" << std::hex << static_cast<ULONG>(diag.ProtectStatus) << "\n";
                std::cout << "  Data page at:     0x" << std::hex << diag.DataPageAddr << "\n";
                std::cout << "=== IMPORT RESOLUTION ===\n";
                std::cout << "  Missing modules:  " << std::dec << diag.MissingModCount << "\n";
                std::cout << "  Unresolved funcs: " << std::dec << diag.UnresolvedCount << "\n";
                std::cout << "  LoadLibraryA:     0x" << std::hex << diag.LoadLibAddr << "\n";
                std::cout << "  GetProcAddress:   0x" << std::hex << diag.GetProcAddr << "\n";
                for (ULONG m = 0; m < diag.MissingModCount && m < 8; m++)
                    std::cout << "  Missing[" << std::dec << m << "]:       " << diag.MissingMod[m] << "\n";
                std::cout << "=== EXCEPTION HANDLING ===\n";
                std::cout << "  Exception entries:  " << std::dec << diag.ExceptionEntries << "\n";
                std::cout << "  RtlAddFuncTable:    0x" << std::hex << diag.RtlAddFuncTableAddr << "\n";
                std::cout << "=== THREAD INFO ===\n";
                std::cout << "  Total threads:    " << std::dec << diag.TotalThreads << "\n";
                std::cout << "  APCs queued:      " << std::dec << diag.ApcCount << "\n";
                std::cout << "=== SHELLCODE PROGRESS ===\n";
                const char* markerStr = "unknown";
                switch (diag.ProgressMarker) {
                    case 0x00: markerStr = "NOT STARTED (crashed before imports?)"; break;
                    case 0x11: markerStr = "imports resolved, crashed before DllMain"; break;
                    case 0x22: markerStr = "exception table registered, crashed in DllMain"; break;
                    case 0x33: markerStr = "DllMain returned OK"; break;
                    case 0xFE: markerStr = "could not map marker page"; break;
                    case 0xFF: markerStr = "marker page inaccessible (process dead?)"; break;
                    default:   markerStr = "unexpected value"; break;
                }
                std::cout << "  Progress marker:  0x" << std::hex << (int)diag.ProgressMarker
                          << " (" << markerStr << ")\n";
                std::cout << "  Process alive:    " << (diag.ProcessAlive ? "YES" : "NO") << "\n";
                std::cout << "==================\n";
            }

            RegDeleteValueW(hKey, L"SessionId");
            RegDeleteValueW(hKey, L"DataPath");
            RegDeleteValueW(hKey, L"Status");
            RegDeleteValueW(hKey, L"DiagData");
            RegCloseKey(hKey);
            return ntstatus < 0 ? 1 : 0;
        }
        std::cout << "." << std::flush;
    }

    std::cerr << "\nTimed out (60s). Driver may not be loaded.\n";
    RegDeleteValueW(hKey, L"SessionId");
    RegDeleteValueW(hKey, L"DataPath");
    RegCloseKey(hKey);
    return 1;
}
