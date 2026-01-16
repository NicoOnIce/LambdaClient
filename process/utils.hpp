#pragma once
#include <windows.h>
#include <string>

DWORD GetRobloxPID();
uintptr_t GetModuleBaseAddress(DWORD pid, const wchar_t* moduleName);