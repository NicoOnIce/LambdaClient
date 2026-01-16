#include <windows.h>

struct WorkSpace {
    int success;
    uintptr_t workSpaceAddress;
};

WorkSpace getWorkSpace(HANDLE hProcess, uintptr_t realDataModelAddress);