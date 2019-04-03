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
#include "../DSMKDriver0/sioctl.h"


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

        if (false == interpreter.InterpretCommand(tokens))
        {
            AppLogError("returned false for command...");
        }
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

#define METHOD_BUFFERED         0
#define METHOD_IN_DIRECT        1
#define FILE_ANY_ACCESS         0

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

static void
_SendIoCtl(
    HANDLE Device,
    DWORD Ioctl,
    BYTE* Buffer,
    DWORD BufferSize,
    BYTE* OutData,
    DWORD OutDataSize
)
{
    DWORD maxSize = max(BufferSize, OutDataSize);
    IOCTL_GET_SET_STRUCT *pIoctl = (IOCTL_GET_SET_STRUCT*)malloc(sizeof(IOCTL_GET_SET_STRUCT) + maxSize);
    DWORD totalSize = sizeof(IOCTL_GET_SET_STRUCT) + maxSize;
    BOOL bRc;
    DWORD bytesRet;

    pIoctl->BufferSize = BufferSize;
    
    if (Buffer != NULL)
    {
        memcpy(pIoctl->Buffer, Buffer, BufferSize);
    }

    bRc = DeviceIoControl(
        Device,
        Ioctl,
        pIoctl,
        totalSize,
        pIoctl,
        totalSize,
        &bytesRet,
        NULL
    );

    if (!bRc)
    {
        AppLogError("IOCTL %x failed!", Ioctl);
        return;
    }

    if (pIoctl->Status != 0)
    {
        AppLogError("IOCTL %x returned %x!", Ioctl, pIoctl->Status);
        return;
    }

    if (NULL != OutData)
    {
        memcpy(OutData, pIoctl->Buffer, OutDataSize);
    }
}


bool SendIoCtlToDrv0(
    std::string IoCtl
)
{
    HANDLE hDevice;
    BOOL bRc;
    ULONG bytesReturned;
    DWORD errNum = 0;
    TCHAR driverLocation[MAX_PATH];
    
    DWORD ioctlTs;

    int ioctl = std::atoi(IoCtl.c_str());

    AppLogInfo("ioctl is %d", ioctl);

    //if (ioctl == 0 || ioctl != 1 || ioctl != 2 || ioctl != 3)
    //{
    //    return false;
    //}

    if (ioctl == 1)
    {      
        ioctlTs = MY_IOCTL_CODE_FIRST;
    }
    else if(ioctl == 2)
    {
        ioctlTs = MY_IOCTL_CODE_SECOND;
    }
    else if (ioctl == 3)
    {
        ioctlTs = MY_IOCTL_CODE_TEST_KTHREAD_POOL;
    }

    if ((hDevice = CreateFileA("\\\\.\\Drv0File",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();
        AppLogError("CreateFile failed : %d\n", errNum);

        return false;
    }

    _SendIoCtl(hDevice, ioctlTs, NULL, 0, NULL, 0);

    CloseHandle(hDevice);

    return true;

}


bool SendProtectIoctl(
    DWORD Pid
)
{
    HANDLE hDevice;
    DWORD ioctlTs;
    DWORD errNum;
    IOCTL_PROT_UNPROT_PROC_STRUCT infoStruct;

    ioctlTs = MY_IOCTL_CODE_PROTECT_PROCESS;

    infoStruct.Pid = Pid;

    if ((hDevice = CreateFileA("\\\\.\\Drv0File",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();
        AppLogError("CreateFile failed : %d\n", errNum);

        return false;
    }

    _SendIoCtl(hDevice, ioctlTs, (BYTE*)&infoStruct, sizeof(IOCTL_PROT_UNPROT_PROC_STRUCT), NULL, 0);

    CloseHandle(hDevice);

    return true;
}


bool SendUnprotectIoctl(
    DWORD Pid
)
{
    HANDLE hDevice;
    DWORD ioctlTs;
    DWORD errNum;
    IOCTL_PROT_UNPROT_PROC_STRUCT infoStruct;

    ioctlTs = MY_IOCTL_CODE_UNPROTECT_PROCESS;

    infoStruct.Pid = Pid;

    if ((hDevice = CreateFileA("\\\\.\\Drv0File",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();
        AppLogError("CreateFile failed : %d\n", errNum);

        return false;
    }

    _SendIoCtl(hDevice, ioctlTs, (BYTE*)&infoStruct, sizeof(IOCTL_PROT_UNPROT_PROC_STRUCT), NULL, 0);

    CloseHandle(hDevice);

    return true;
}

bool MakeRemoteThread(
    DWORD Pid
)
{
    HANDLE hnd, tHnd;

    hnd = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, Pid);

    if (hnd == INVALID_HANDLE_VALUE)
    {
        AppLogError("OpenProcess failed: 0x%08x\n", GetLastError());
        return false;
    }

    tHnd = CreateRemoteThread(
        hnd,
        NULL,
        0,
        0,
        NULL,
        0,
        NULL
    );

    if (NULL == tHnd)
    {
        AppLogError("CreateRemoteThread failed: 0x%08x\n", GetLastError());
        return false;
    }

    return true;

}


bool SendIoCtlToDrv1(
    std::string IoCtl
)
{
    HANDLE hDevice;
    BOOL bRc;
    ULONG bytesReturned;
    DWORD errNum = 0;
    TCHAR driverLocation[MAX_PATH];
    CHAR inputBuffer[100];
    CHAR outputBuffer[100];
    DWORD ioctlTs;

    int ioctl = std::atoi(IoCtl.c_str());

    if (ioctl == 0 || (ioctl != 1 && ioctl != 2))
    {
        return false;
    }

    if (ioctl == 1)
    {
        ioctlTs = MY_IOCTL_CODE_FIRST;
    }
    else if(ioctl == 2)
    {
        ioctlTs = MY_IOCTL_CODE_SECOND;
    }

    if ((hDevice = CreateFileA("\\\\.\\Drv1File",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE) {

        errNum = GetLastError();
        AppLogError("CreateFile failed : %d\n", errNum);

        return false;
    }

    ZeroMemory(inputBuffer, sizeof(inputBuffer));
    strcpy_s(inputBuffer, "abcd");
    ZeroMemory(outputBuffer, sizeof(outputBuffer));

    bRc = DeviceIoControl(hDevice,
        (DWORD)ioctlTs,
        &inputBuffer,
        (DWORD)strlen(inputBuffer) + 1,
        &outputBuffer,
        sizeof(outputBuffer),
        &bytesReturned,
        NULL
    );

    if (!bRc)
    {
        AppLogError("Error in DeviceIoControl : %d", GetLastError());
        return false;

    }
    AppLogInfo("    OutBuffer (%d): %s\n", bytesReturned, outputBuffer);


    CloseHandle(hDevice);

    return true;

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

        if (argv[i].compare(0, sizeof("sendioctl"), "sendioctl") == 0)
        {
            if (i + 1 >= argc)
            {
                std::cout << "sendioctl <ioctl>\n";
                return false;
            }

            return SendIoCtlToDrv0(argv[i + 1]);
        }

        if (argv[i].compare(0, sizeof("protect"), "protect") == 0)
        {
            if (i + 1 >= argc)
            {
                std::cout << "protect <pid>\n";
                return false;
            }

            return SendProtectIoctl(std::atoi(argv[i + 1].c_str()));
        }

        if(argv[i].compare(0, sizeof("unprotect"), "unprotect") == 0)
        {
            if (i + 1 >= argc)
            {
                std::cout << "unprotect <pid>\n";
                return false;
            }

            return SendUnprotectIoctl(std::atoi(argv[i + 1].c_str()));
        }

        if (argv[i].compare(0, sizeof("testpool"), "testpool") == 0)
        {
            return SendIoCtlToDrv0("3");
        }

        if (argv[i].compare(0, sizeof("sendioctl1"), "sendioctl1") == 0)
        {
            if (i + 1 >= argc)
            {
                std::cout << "sendioctl1 <ioctl>\n";
                return false;
            }

            return SendIoCtlToDrv1(argv[i + 1]);
        }

        if (argv[i].compare(0, sizeof("remote"), "remote") == 0)
        {
            if (i + 1 >= argc)
            {
                std::cout << "remote <pid>\n";
                return false;
            }

            return MakeRemoteThread(std::atoi(argv[i + 1].c_str()));
        }

        std::cout<<"unknown command!\n";

        AppLogError("Unable to process command: %s", argv[i].c_str());

        return false;
    }

    return true;
}