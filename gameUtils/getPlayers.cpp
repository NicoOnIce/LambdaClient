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

struct Players {
    int success;
    uintptr_t playersAddress;
};

Players getPlayers(HANDLE hProcess, uintptr_t realDataModelAddress) {
    Players toReturn;
    toReturn.success = 1;

    auto childrenOfDataModel = getChildren(hProcess, realDataModelAddress);
    int playersIndex = findFirstChildOfByName(childrenOfDataModel, hProcess, "Players");

    if (playersIndex == -1) {
        return toReturn;
    }

    auto playersAddress = childrenOfDataModel[playersIndex];
    toReturn.playersAddress = playersAddress;

    toReturn.success = 0;
    return toReturn;
}