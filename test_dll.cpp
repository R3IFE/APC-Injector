#include <windows.h>

#pragma comment(linker, "/ENTRY:DllMain")
#pragma comment(linker, "/NODEFAULTLIB")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")

static volatile LONG g_shown = 0;

extern "C" BOOL __stdcall DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        if (InterlockedCompareExchange(&g_shown, 1, 0) == 0) {
            HWND hwnd = FindWindowA("Notepad", NULL);
            if (hwnd) SetWindowTextA(hwnd, "*** INJECTED via kernel APC ***");

            HANDLE hf = CreateFileA(
                "C:\\Windows\\Temp\\inj_msg.vbs",
                GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);
            if (hf != INVALID_HANDLE_VALUE) {
                char vbs[256];
                int len = wsprintfA(vbs,
                    "MsgBox \"test_dll injected via kernel-mode APC into PID %lu.\","
                    " 64, \"Kernel Injector Test\"\r\n",
                    GetCurrentProcessId());
                DWORD w = 0;
                WriteFile(hf, vbs, len, &w, NULL);
                CloseHandle(hf);

                STARTUPINFOA si = {};
                si.cb = sizeof(si);
                PROCESS_INFORMATION pi = {};
                char cmd[] = "wscript.exe C:\\Windows\\Temp\\inj_msg.vbs";
                if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE,
                                    CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
    }
    return TRUE;
}
