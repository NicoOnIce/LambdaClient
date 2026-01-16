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

struct Humanoid {
    int success;
    uintptr_t humanoidAddress;
};

Humanoid getHumanoidOfPlayer(HANDLE hProcess, uintptr_t workSpaceAddress, std::string playerName) {
    Humanoid toReturn;
    toReturn.success = 1;

    auto workSpaceChildren = getChildren(hProcess, workSpaceAddress);
    int playerCharacterIndex = findFirstChildOfByName(workSpaceChildren, hProcess, trim(playerName));

    if (playerCharacterIndex == -1) {
        return toReturn;
    }

    auto playerCharacterChildren = getChildren(hProcess, workSpaceChildren[playerCharacterIndex]);

    int playerHumanoidIndex = findFirstChildOfByName(playerCharacterChildren, hProcess, "Humanoid");

    if (playerHumanoidIndex == -1) {
        return toReturn;
    }

    auto playerHumanoidAddress = playerCharacterChildren[playerHumanoidIndex];
    toReturn.humanoidAddress = playerHumanoidAddress;

    toReturn.success = 0;
    return toReturn;
}