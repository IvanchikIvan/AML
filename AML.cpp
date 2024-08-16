#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <string>

SIZE_T GetTotalMemoryUsageByName(const std::wstring& processName) {
    DWORD processes[1024], bytesReturned;

    if (!EnumProcesses(processes, sizeof(processes), &bytesReturned)) {
        std::wcerr << L"Failed to enumerate processes." << std::endl;
        return 0;
    }

    SIZE_T totalMemoryUsage = 0;
    int numProcesses = bytesReturned / sizeof(DWORD);

    for (int i = 0; i < numProcesses; i++) {
        if (processes[i] == 0) continue;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);
        if (hProcess) {
            wchar_t processNameBuffer[MAX_PATH];
            if (GetModuleBaseNameW(hProcess, nullptr, processNameBuffer, sizeof(processNameBuffer) / sizeof(wchar_t))) {
                if (processName == processNameBuffer) {
                    PROCESS_MEMORY_COUNTERS_EX pmc;
                    if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                        totalMemoryUsage += pmc.WorkingSetSize;
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }

    return totalMemoryUsage;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) {
        std::wcerr << L"Usage: " << argv[0] << L" <process_name> <memory_limit_MB>" << std::endl;
        return 1;
    }

    std::wstring processName = argv[1];
    SIZE_T memoryLimitBytes = std::stoul(argv[2]) * 1024 * 1024;

    while (true) {
        SIZE_T totalMemoryUsage = GetTotalMemoryUsageByName(processName);

        std::wcout << L"Total memory usage by " << processName << L": " << totalMemoryUsage / 1024 << L" KB" << std::endl;

        if (totalMemoryUsage > memoryLimitBytes) {
            std::wcerr << L"Memory limit exceeded!" << std::endl;
            DWORD processes[1024], bytesReturned;

            if (EnumProcesses(processes, sizeof(processes), &bytesReturned)) {
                int numProcesses = bytesReturned / sizeof(DWORD);

                for (int i = 0; i < numProcesses; i++) {
                    if (processes[i] == 0) continue;

                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, processes[i]);
                    if (hProcess) {
                        wchar_t processNameBuffer[MAX_PATH];
                        if (GetModuleBaseNameW(hProcess, nullptr, processNameBuffer, sizeof(processNameBuffer) / sizeof(wchar_t))) {
                            if (processName == processNameBuffer) {
                                TerminateProcess(hProcess, 1);
                            }
                        }
                        CloseHandle(hProcess);
                    }
                }
            }
            break;
        }

        Sleep(1000);
    }

    return 0;
}
