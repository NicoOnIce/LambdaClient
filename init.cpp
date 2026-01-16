#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <format>

#include "offsets/offsets.hpp"
#include "process/utils.hpp"
#include "utils/strings.hpp"
#include "utils/getChildren.hpp"

#include "memory/memory.hpp"

struct out {
    DWORD pid;
    HANDLE hProcess;

    uintptr_t base;
    uintptr_t realDataModel;

    int success;
};

out debugInit() {
    out toReturn;
    toReturn.success = 1;

    DWORD pid = GetRobloxPID();
    if (!pid) {
        std::cerr << "failed to find PID of roblox process" << std::endl;
        return toReturn;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        std::cerr << "failed to open roblox process" << std::endl;
        return toReturn;
    }

    uintptr_t base = GetModuleBaseAddress(pid, L"RobloxPlayerBeta.exe");
    if (!base) {
        std::cerr << "failed to resolve base address" << std::endl;
        return toReturn;
    }

    auto fakeDataModel = ReadMemory<uintptr_t>(hProcess, base + Offsets::FakeDataModel::Pointer);
    std::cout << "Fake Data Model pointer: 0x" << std::hex << fakeDataModel << std::endl;
    
    auto realDataModel = ReadMemory<uintptr_t>(hProcess, fakeDataModel + Offsets::FakeDataModel::RealDataModel);
    std::cout << "Real Data Model pointer: 0x" << std::hex << realDataModel << std::endl;
    toReturn.realDataModel = realDataModel;

    std::cout << "====================================================" << std::endl;
    auto childrenOfDataModel = getChildren(hProcess, realDataModel);
    std::cout << "====================================================" << std::endl;

    int workSpaceIndex = findFirstChildOfByName(childrenOfDataModel, hProcess, "Workspace");
    int playerIndex = findFirstChildOfByName(childrenOfDataModel, hProcess, "Players");

    if (workSpaceIndex == -1 || playerIndex == -1) {
        return toReturn;
    }

    std::cout << "\n\n\n\n====================================================" << std::endl;
    auto players = getChildren(hProcess, childrenOfDataModel[playerIndex]);
    std::cout << "====================================================" << std::endl;

    std::cout << "\n\n\n\n====================================================" << std::endl;
    auto player = players[0];
    auto playerName = getChildName(hProcess, player);

    auto workSpaceChildren = getChildren(hProcess, childrenOfDataModel[workSpaceIndex]);
    int playerCharacterIndex = findFirstChildOfByName(workSpaceChildren, hProcess, trim(playerName));
    std::cout << "====================================================" << std::endl;

    std::cout << "\n\n\n\n====================================================" << std::endl;
    auto playerCharacterChildren = getChildren(hProcess, workSpaceChildren[playerCharacterIndex]);
    std::cout << "====================================================" << std::endl;

    std::cout << "\n\n\n\n====================================================" << std::endl;
    int playerHumanoidIndex = findFirstChildOfByName(playerCharacterChildren, hProcess, "Humanoid");
    // auto playerHumanoidChildren = getChildren(hProcess, playerCharacterChildren[playerHumanoidIndex]);
    std::cout << "====================================================" << std::endl;

    std::cout << "\n\n\n\n====================================================" << std::endl;
    float direct = ReadMemory<float>(hProcess, playerCharacterChildren[playerHumanoidIndex] + 0x18C);
    std::cout << "player health: " << direct << "\n";
    std::cout << "====================================================" << std::endl;

    toReturn.success = 0;
    return toReturn;
}

out init() {
    out toReturn;
    toReturn.success = 1;

    DWORD pid = GetRobloxPID();
    if (!pid) {
        std::cerr << "failed to find PID of roblox process" << std::endl;
        return toReturn;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) {
        std::cerr << "failed to open roblox process" << std::endl;
        return toReturn;
    }

    toReturn.hProcess = hProcess;

    uintptr_t base = GetModuleBaseAddress(pid, L"RobloxPlayerBeta.exe");
    if (!base) {
        std::cerr << "failed to resolve base address" << std::endl;
        return toReturn;
    }

    toReturn.base = base;

    auto fakeDataModel = ReadMemory<uintptr_t>(hProcess, base + Offsets::FakeDataModel::Pointer);
    
    auto realDataModel = ReadMemory<uintptr_t>(hProcess, fakeDataModel + Offsets::FakeDataModel::RealDataModel);
    toReturn.realDataModel = realDataModel;

    toReturn.success = 0;
    return toReturn;
}