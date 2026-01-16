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

#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

#include "memory/memory.hpp"

struct WorkSpace {
    int success;
    uintptr_t workSpaceAddress;
};

WorkSpace getWorkSpace(HANDLE hProcess, uintptr_t realDataModelAddress) {
    WorkSpace toReturn;
    toReturn.success = 1;

    auto childrenOfDataModel = getChildren(hProcess, realDataModelAddress);
    int workSpaceIndex = findFirstChildOfByName(childrenOfDataModel, hProcess, "Workspace");

    if (workSpaceIndex == -1) {
        return toReturn;
    }

    auto workSpaceAddress = childrenOfDataModel[workSpaceIndex];
    toReturn.workSpaceAddress = workSpaceAddress;

    toReturn.success = 0;
    return toReturn;
}