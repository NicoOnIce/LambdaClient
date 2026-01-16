#include <windows.h>

#ifndef PLAYERS_HPP
#define PLAYERS_HPP
struct Players {
    int success;
    uintptr_t playersAddress;
};
#endif

Players getPlayers(HANDLE hProcess, uintptr_t realDataModelAddress);