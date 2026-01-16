#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

#include "offsets/offsets.hpp"
#include "process/utils.hpp"
#include "utils/strings.hpp"

#include "memory/memory.hpp"

std::vector<uintptr_t> debugGetChildren(HANDLE hProcess, uintptr_t address) {
    std::vector<uintptr_t> children;

    if (!address)
        return children;

    uintptr_t children_struct_ptr = ReadMemory<uintptr_t>(hProcess, address + 0x68);

    if (!children_struct_ptr) {
        std::cout << "failed to resolve children structure pointer" << std::endl;
        return children;
    }

    std::cout << "resolved children structure pointer sucessfuly" << std::endl;
  
    uintptr_t childrenStart = ReadMemory<uintptr_t>(hProcess, children_struct_ptr);
    uintptr_t childrenEnd = ReadMemory<uintptr_t>(hProcess, address + 0x8);

    while (childrenStart && childrenStart != childrenEnd) {
        uintptr_t child_address = ReadMemory<uintptr_t>(hProcess, childrenStart);
        if (!child_address)
            break;

        std::cout << "resolved new child" << std::endl;

        children.push_back(child_address);

        childrenStart += 0x10;
    }

    return children;
}

std::string debugGetChildName(HANDLE hProcess, uintptr_t address) {
    char buffer[128] = {0};

    uintptr_t nameAddress = ReadMemory<uintptr_t>(hProcess, address + 0xA8);
    std::cout << "name pointer: 0x" << std::hex << nameAddress << std::endl;

    ReadProcessMemory(hProcess, (LPCVOID)nameAddress, buffer, sizeof(buffer), nullptr);
    std::string name(buffer);
    std::cout << "name value: " << buffer << std::endl;

    return name;
}

int debugFindFirstChildOfByName(std::vector<uintptr_t> vector, HANDLE hProcess, std::string target) {
    for (int i=0; vector.size() > i; i++) {
        std::cout << "====================================================" << std::endl;
        std::string currentChildName = debugGetChildName(hProcess, vector[i]);

        std::cout << "Length: " << currentChildName.length() << std::endl;

        if (trim(currentChildName) == target) {
            std::cout << "resolved target pointer" << std::endl;
            return i;
        }

        std::cout << "====================================================" << std::endl;
    }

    return -1;
}

std::vector<uintptr_t> getChildren(HANDLE hProcess, uintptr_t address) {
    std::vector<uintptr_t> children = std::vector<uintptr_t>{};

    if (!address)
        return children;

    uintptr_t children_struct_ptr = ReadMemory<uintptr_t>(hProcess, address + 0x68);

    if (!children_struct_ptr) {
        std::cout << "failed to resolve children structure pointer" << std::endl;
        return children;
    }
  
    uintptr_t childrenStart = ReadMemory<uintptr_t>(hProcess, children_struct_ptr);
    uintptr_t childrenEnd = ReadMemory<uintptr_t>(hProcess, address + 0x8);

    while (childrenStart && childrenStart != childrenEnd) {
        uintptr_t child_address = ReadMemory<uintptr_t>(hProcess, childrenStart);
        if (!child_address)
            break;

        children.push_back(child_address);

        childrenStart += 0x10;
    }

    return children;
}

std::string getChildName(HANDLE hProcess, uintptr_t address) {
    char buffer[128] = {0};

    uintptr_t nameAddress = ReadMemory<uintptr_t>(hProcess, address + 0xA8);

    ReadProcessMemory(hProcess, (LPCVOID)nameAddress, buffer, sizeof(buffer), nullptr);
    std::string name(buffer);

    return name;
}

int findFirstChildOfByName(std::vector<uintptr_t> vector, HANDLE hProcess, std::string target) {
    for (int i=0; vector.size() > i; i++) {
        std::string currentChildName = getChildName(hProcess, vector[i]);

        if (trim(currentChildName) == target) {
            return i;
        }

    }

    return -1;
}