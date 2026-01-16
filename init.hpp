struct out {
    DWORD pid;
    HANDLE hProcess;

    uintptr_t base;
    uintptr_t realDataModel;

    int success;
};

out debugInit();
out init();