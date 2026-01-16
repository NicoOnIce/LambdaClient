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

struct Character {
    int success;
    uintptr_t characterAddress;
};

Character getCharacterOfPlayer(HANDLE hProcess, uintptr_t workSpaceAddress, std::string playerName) {
    Character toReturn;
    toReturn.success = 1;

    auto workSpaceChildren = getChildren(hProcess, workSpaceAddress);

    int playerCharacterIndex = findFirstChildOfByName(workSpaceChildren, hProcess, trim(playerName));

    if (playerCharacterIndex == -1) {
        return toReturn;
    }
    
    auto playerCharacterAddress = workSpaceChildren[playerCharacterIndex];
    
    toReturn.characterAddress = playerCharacterAddress;
    toReturn.success = 0;
    return toReturn;
}