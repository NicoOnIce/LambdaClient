#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

template<typename T>
T ReadMemory(HANDLE hProcess, uintptr_t address) {
    T buffer{};
    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address),
                           &buffer, sizeof(T), &bytesRead))
    {
        // DWORD error = GetLastError();
        // std::cerr << "[ReadMemory] Failed to read at address 0x"
        //           << std::hex << address
        //           << " | Error: " << std::dec << error
        //           << " (" << std::hex << error << " hex)\n";
    }
    // else if (bytesRead != sizeof(T))
    // {
    //     std::cerr << "[ReadMemory] Partial read at address 0x"
    //               << std::hex << address
    //               << " | Expected: " << sizeof(T)
    //               << " bytes, Got: " << bytesRead << "\n";
    // }

    return buffer;
}