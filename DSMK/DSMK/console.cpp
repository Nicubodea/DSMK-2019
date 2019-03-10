#include <iostream>

#include "trace.h"
#include "./x64/Debug/console.tmh"

#include <sstream>
#include <string>
#include <iterator>
#include <vector>

#include <psapi.h>
#include <winternl.h>

#include "console.h"


bool gStop = false;

typedef NTSTATUS(NTAPI *PFUNC_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

PFUNC_NtQueryInformationProcess pNtQueryInformationProcess = NULL;


int main()
{
    WPP_INIT_TRACING(NULL);

    CommandInterpreter interpreter;
    char buff[101];
    HMODULE hMod;

    // ntdll.dll is already loaded, but this is how MS documents using NtQueryInformationProces...
    hMod = LoadLibrary(L"ntdll.dll");
    if (NULL == hMod)
    {
        DWORD lasterr = GetLastError();
        
        std::cout << "Will not be able to find command line of process due to failure of LoadLibrary: " << lasterr;
        
        AppLogError("ntdll.dll could not be loaded: 0x%08x, will not get command lines...", lasterr);

        goto no_command_line;
    }

    pNtQueryInformationProcess = (PFUNC_NtQueryInformationProcess)GetProcAddress(hMod, "NtQueryInformationProcess");
    if (NULL == pNtQueryInformationProcess)
    {
        DWORD lasterr = GetLastError();

        std::cout << "Will not be able to find command line of process due to failure of GetProcAddress: " << lasterr;

        AppLogError("GetProcAddress failed: 0x%08x, will not get command lines...", lasterr);
    }

no_command_line:

    if (NULL != hMod)
    {
        FreeLibrary(hMod);
    }

    while (!gStop)
    {
        std::cout << ">";

        std::cin.getline(buff, 100);

        std::istringstream buf(buff);
        std::istream_iterator<std::string> beg(buf), end;

        std::vector<std::string> tokens(beg, end);

        std::cout << "Command line: " << buff << "\n";

        AppLogInfo("Command given: %s", buff);

        interpreter.InterpretCommand(tokens);        
    }

    WPP_CLEANUP();
}



bool GetCommandLineForProcess(
    _In_ DWORD Pid,
    _In_opt_ HANDLE Handle
)
{
    HANDLE hProc = NULL;
    PROCESS_BASIC_INFORMATION procInfo;
    DWORD retLen;
    NTSTATUS status;
    bool ret = true;
    UNICODE_STRING str;
    RTL_USER_PROCESS_PARAMETERS params;
    RTL_USER_PROCESS_PARAMETERS *pParams;
    SIZE_T readLen;
    WCHAR cmdLine[1024];
    DWORD currentIndex = 0;
    bool shouldCloseHandle = false;

    if (NULL == pNtQueryInformationProcess)
    {
        return false;
    }

    if (NULL == Handle)
    {
        hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Pid);
        if (NULL == hProc)
        {
            AppLogError("OpenProcess failed: 0x%08x", GetLastError());
            return false;
        }
        shouldCloseHandle = true;
    }
    else
    {
        hProc = Handle;
    }


    status = pNtQueryInformationProcess(hProc, ProcessBasicInformation, &procInfo, sizeof(procInfo), &retLen);
    if (!NT_SUCCESS(status))
    {
        AppLogError("NtQueryInformation process failed for pid %d: 0x%08x", Pid, status);
        ret = false;
        goto cleanup_and_exit;
    }

    AppLogInfo("NtQueryInformationProcess success!");
    if (!ReadProcessMemory(hProc, &procInfo.PebBaseAddress->ProcessParameters, &pParams, sizeof(pParams), &readLen))
    {
        AppLogError("ReadProcessMemory process failed for pid %d: 0x%08x", Pid, GetLastError());
        ret = false;
        goto cleanup_and_exit;
    }

    AppLogInfo("Successfully read process params: %llx", readLen);

    if (!ReadProcessMemory(hProc, pParams, &params, sizeof(params), &readLen))
    {
        AppLogError("ReadProcessMemory process failed for pid %d: 0x%08x", Pid, GetLastError());
        ret = false;
        goto cleanup_and_exit;
    }

    AppLogInfo("Total bytes to read: %d", params.CommandLine.Length);

    while (params.CommandLine.Length > 0)
    {
        if (!ReadProcessMemory(hProc, ((PWCHAR)params.CommandLine.Buffer) + currentIndex, cmdLine, min(sizeof(cmdLine) - 2, params.CommandLine.Length), &readLen))
        {
            AppLogError("ReadProcessMemory process failed for pid %d: 0x%08x", Pid, GetLastError());
            ret = false;
            goto cleanup_and_exit;
        }

        AppLogInfo("Successfully read process command line: %llx", readLen);

        cmdLine[readLen / 2] = 0;

        AppLogInfo("Command line: %S", cmdLine);

        std::wcout << cmdLine;

        params.CommandLine.Length -= (USHORT)readLen;
        currentIndex += (DWORD)readLen / 2;
    }

cleanup_and_exit:
    if (shouldCloseHandle)
    {
        CloseHandle(hProc);
    }

    return ret;
}


void PrintAllProcesses()
{
    DWORD procBuff[512];
    DWORD retSz = 0;
    DWORD i;
    DWORD arrSz;
    HANDLE hProc;
    WCHAR pProcName[512];
    DWORD nameRetSz = 0;

    do
    {
        if (!EnumProcesses(procBuff, sizeof(procBuff), &retSz))
        {
            DWORD lasterr = GetLastError();
            std::cerr << "ERROR: EnumProcesses failed: " << lasterr << "\n";

            AppLogError("EnumProcesses failed: 0x%08x", lasterr);
            goto exit_function;
        }

        arrSz = retSz / sizeof(DWORD);

        for (i = 0; i < arrSz; i++)
        {
            if (procBuff[i] == 0)
            {
                continue;
            }
            hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procBuff[i]);
            if (NULL == hProc)
            {
                DWORD lasterr = GetLastError();
                //std::cerr << "ERROR: OpenProcess failed for pid " << procBuff[i] << ": " << lasterr << "\n";

                AppLogError("OpenProcess failed for pid: %d, error: 0x%08x", procBuff[i], lasterr);
                continue;
            }

            nameRetSz = GetModuleBaseName(hProc, NULL, pProcName, sizeof(pProcName) / 2 - 2);
            if (0 == nameRetSz)
            {
                DWORD lasterr = GetLastError();
                std::cerr << "ERROR: GetModuleBaseName failed for pid " << procBuff[i] << ": " << lasterr << "\n";

                AppLogError("GetModuleBaseName failed for pid: %d, error: 0x%08x", procBuff[i], lasterr);
                CloseHandle(hProc);
                continue;
            }

            AppLogInfo("Found process: %S pid %d", pProcName, procBuff[i]);
            
            std::wcout << procBuff[i] << ": " << pProcName << ", command line: ";

            if (!GetCommandLineForProcess(procBuff[i], hProc))
            {
                std::cout << "<no command line could be get, check wpp logs>";
            }

            std::cout << "\n";

            CloseHandle(hProc);
        }

    } while (retSz == sizeof(procBuff));

exit_function:
    return;
}


bool CommandInterpreter::InterpretCommand(
    _In_ std::vector<std::string> argv
)
{
    int argc = argv.size();
    AppLogTrace("argc = %d", argc);

    for (int i = 0; i < argc; i++)
    {
        AppLogTrace("argv[%d] = %s", i, argv[i].c_str());

        if (argv[i].compare(0, sizeof("help"), "help") == 0)
        {
            std::cout << "The following commands are available: \n";

            std::cout << "help      - prints the help of the console\n";

            std::cout << "proclst   - prints info about processes on the system (PID, name, command line)\n";

            std::cout << "exit      - exits the console\n";

            return true;
        }

        if (argv[i].compare(0, sizeof("exit"), "exit") == 0)
        {
            std::cout << "exit was given\n";

            gStop = 1;

            return true;
        }

        if (argv[i].compare(0, sizeof("start"), "start") == 0)
        {
            std::cout << "starting thread pool\n";

            tp->StartThreadPool();

            return true;
        }

        if (argv[i].compare(0, sizeof("stop"), "stop") == 0)
        {
            std::cout << "starting thread pool\n";

            tp->StopThreadPool();

            return true;
        }

        if (argv[i].compare(0, sizeof("proclst"), "proclst") == 0)
        {
            std::cout << "showing processes: \n";

            PrintAllProcesses();
            
            return true;
        }

        std::cout<<"unknown command!\n";

        AppLogError("Unable to process command: %s", argv[i].c_str());

        return false;
    }

    return true;
}