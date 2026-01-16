#include <windows.h>
#include <string>

struct Character {
    int success;
    uintptr_t characterAddress;
};

Character getCharacterOfPlayer(HANDLE hProcess, uintptr_t workSpaceAddress, std::string playerName);