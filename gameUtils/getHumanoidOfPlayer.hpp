#include <windows.h>

#ifndef HUMANOID_HPP
#define HUMANOID_HPP
struct Humanoid {
    int success;
    uintptr_t humanoidAddress;
};
#endif

Humanoid getHumanoidOfPlayer(HANDLE hProcess, uintptr_t workSpaceAddress, std::string playerName);