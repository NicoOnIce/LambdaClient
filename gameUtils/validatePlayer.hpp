#include <windows.h>
#include <string>
#include <regex>

bool isValidPlayer(const std::string& name, HANDLE hProcess, const std::vector<uintptr_t>& workSpaceChildren);