#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

std::vector<uintptr_t> getChildren(HANDLE hProcess, uintptr_t address);
std::string getChildName(HANDLE hProcess, uintptr_t address);
int findFirstChildOfByName(std::vector<uintptr_t> vector, HANDLE hProcess, std::string target);

std::vector<uintptr_t> debugGetChildren(HANDLE hProcess, uintptr_t address);
std::string debugGetChildName(HANDLE hProcess, uintptr_t address);
int debugFindFirstChildOfByName(std::vector<uintptr_t> vector, HANDLE hProcess, std::string target);