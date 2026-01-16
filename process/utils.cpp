#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

DWORD GetRobloxPID() {
    DWORD pid = 0;
    HWND hwnd = FindWindowA(NULL, "Roblox");
    if (hwnd) {
        GetWindowThreadProcessId(hwnd, &pid);
    }
    return pid;
}

uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName) {
    uintptr_t baseAddress = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W module;
        module.dwSize = sizeof(MODULEENTRY32W);
        if (Module32FirstW(snapshot, &module)) {
            do {
                if (_wcsicmp(module.szModule, moduleName) == 0) {
                    baseAddress = (uintptr_t)module.modBaseAddr;
                    break;
                }
            } while (Module32NextW(snapshot, &module));
        }
    }
    CloseHandle(snapshot);
    return baseAddress;
}