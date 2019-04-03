#include <fltKernel.h>

#include "protproc.h"

#include "trace.h"

#include "./x64/Debug/protproc.tmh"

LIST_ENTRY gProtectedProcesses;
KMUTEX gProtProcMutex;
PVOID gRegistrationHandle = NULL;
BOOLEAN bShouldObUnregister = FALSE;
BOOLEAN bShouldUnregisterThread = FALSE;

#define PROT_PROC_TAG   'ORPP'

typedef struct _PROT_PROC_INFO
{
    LIST_ENTRY Link;
    DWORD Pid;
} PROT_PROC_INFO, *PPROT_PROC_INFO;

OB_CALLBACK_REGISTRATION gCbRegistration = { 0 };
OB_OPERATION_REGISTRATION gOpRegistrations[2] = { {0}, {0} };

UNICODE_STRING gAltitude;

static BOOLEAN
_Drv0IsProtected(
    _In_ DWORD Pid
)
{
    LIST_ENTRY* list;
    BOOLEAN found = FALSE;

    KeWaitForSingleObject(&gProtProcMutex, Executive, KernelMode, FALSE, NULL);

    list = gProtectedProcesses.Flink;

    while (list != &gProtectedProcesses)
    {
        PROT_PROC_INFO* pProtInfo = CONTAINING_RECORD(list, PROT_PROC_INFO, Link);

        list = list->Flink;

        if (pProtInfo->Pid == Pid)
        {
            found = TRUE;
        }
    }

    KeReleaseMutex(&gProtProcMutex, FALSE);

    return found;
}


static void
_Drv0ThreadCreateNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    PVOID backTrace[5] = { 0 };

    UNREFERENCED_PARAMETER(ThreadId);

    if (!Create)
    {
        goto exit;
    }

    if (!_Drv0IsProtected((DWORD)(SIZE_T)ProcessId))
    {
        goto exit;
    }

    if (ProcessId == PsGetCurrentProcessId())
    {
        goto exit;
    }

    Drv0LogInfo("[THR-PROT] Possible CreateRemoteThread injection from %d to %d", 
        (DWORD)(SIZE_T)PsGetCurrentProcessId(), (DWORD)(SIZE_T)ProcessId);

    RtlCaptureStackBackTrace(
        0,
        5,
        backTrace,
        NULL
    );

    for (int i = 0; i < 5; i++)
    {
        Drv0LogInfo("[THR-STK] %p\n", backTrace[i]);
    }

exit:
    return;
}


static OB_PREOP_CALLBACK_STATUS
_Drv0PreopProcessCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    UNREFERENCED_PARAMETER(RegistrationContext);
    DWORD currentProcessPid;
    DWORD operationPid;
    ACCESS_MASK old;
    ACCESS_MASK new;
    
    if (OperationInformation->ObjectType != *PsProcessType)
    {
        Drv0LogError("[ERROR] _Drv0PreopProcessCallback called with object type != PsProcessType!\n");
        goto exit;
    }

    operationPid = (DWORD)(SIZE_T)PsGetProcessId(OperationInformation->Object);
    currentProcessPid = (DWORD)(SIZE_T)PsGetCurrentProcessId();

    if (!_Drv0IsProtected(operationPid))
    {
        goto exit;
    }

    if (operationPid == currentProcessPid)
    {
        goto exit;
    }

    if (OperationInformation->KernelHandle == 1)
    {
        goto exit;
    }

    if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        old = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
        OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= (~1);
        new = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
    }
    else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE)
    {
        old = OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
        OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= (~1);
        new = OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
    }
    else
    {
        Drv0LogError("[ERROR] Invalid operation %x!\n", OperationInformation->Operation);
        goto exit;
    }

    if (old != new)
    {
        Drv0LogInfo("[KILL-PROT] Blocking from %d to %d! (old %x new %x)\n", currentProcessPid, operationPid, old, new);
    }
    

    exit:
    return OB_PREOP_SUCCESS;
}

static OB_PREOP_CALLBACK_STATUS
_Drv0PreopThreadCallback(
    _In_ PVOID RegistrationContext,
    _In_ POB_PRE_OPERATION_INFORMATION OperationInformation
)
{
    DWORD operationPid, currentProcessPid;
    ACCESS_MASK old, new;

    UNREFERENCED_PARAMETER(RegistrationContext);
    UNREFERENCED_PARAMETER(OperationInformation);

    if (OperationInformation->ObjectType != *PsThreadType)
    {
        Drv0LogError("[ERROR] _Drv0PreopProcessCallback called with object type != PsProcessType!\n");
        goto exit;
    }

    operationPid = (DWORD)(SIZE_T)PsGetThreadProcessId(OperationInformation->Object);
    currentProcessPid = (DWORD)(SIZE_T)PsGetCurrentProcessId();

    if (!_Drv0IsProtected(operationPid))
    {
        goto exit;
    }

    if (operationPid == currentProcessPid)
    {
        goto exit;
    }

    if (OperationInformation->KernelHandle == 1)
    {
        goto exit;
    }

    if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        old = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
        OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= (~1);
        new = OperationInformation->Parameters->CreateHandleInformation.DesiredAccess;
    }
    else if (OperationInformation->Operation == OB_OPERATION_HANDLE_DUPLICATE)
    {
        old = OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
        OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess &= (~1);
        new = OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess;
    }
    else
    {
        Drv0LogError("[ERROR] Invalid operation %x!\n", OperationInformation->Operation);
        goto exit;
    }

    if (old != new)
    {
        Drv0LogInfo("[KILL-THR] Blocking from %d to %d! (old %x new %x)\n", currentProcessPid, operationPid, old, new);
    }


exit:
    return OB_PREOP_SUCCESS;
}


NTSTATUS
Drv0InitializeProcessProtection(
    VOID
)
{
    NTSTATUS status;

    InitializeListHead(&gProtectedProcesses);

    KeInitializeMutex(&gProtProcMutex, 0);

    gOpRegistrations[0].ObjectType = PsProcessType;
    gOpRegistrations[0].Operations = OB_OPERATION_HANDLE_DUPLICATE | OB_OPERATION_HANDLE_CREATE;
    gOpRegistrations[0].PreOperation = _Drv0PreopProcessCallback;

    gOpRegistrations[1].ObjectType = PsThreadType;
    gOpRegistrations[1].Operations |= OB_OPERATION_HANDLE_DUPLICATE | OB_OPERATION_HANDLE_CREATE;
    gOpRegistrations[1].PreOperation = _Drv0PreopThreadCallback;

    RtlInitUnicodeString(&gAltitude, u"320000");

    gCbRegistration.Version = OB_FLT_REGISTRATION_VERSION;
    gCbRegistration.OperationRegistrationCount = 2;
    gCbRegistration.OperationRegistration = gOpRegistrations;
    gCbRegistration.Altitude = gAltitude;
    gCbRegistration.RegistrationContext = NULL;

    status = ObRegisterCallbacks(&gCbRegistration, &gRegistrationHandle);
    if (!NT_SUCCESS(status))
    {
        Drv0LogError("[ERROR] ObRegisterCallbacks failed: 0x%08x\n", status);
        return status;
    }

    bShouldObUnregister = TRUE;

    status = PsSetCreateThreadNotifyRoutine(
        _Drv0ThreadCreateNotifyRoutine
    );
    if (!NT_SUCCESS(status))
    {
        Drv0LogError("[ERROR] PsSetCreateThreadNotifyRoutine failed: 0x%08x\n", status);
        return status;
    }

    bShouldUnregisterThread = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
Drv0UnitializeProcessProtection(
    VOID
)
{
    LIST_ENTRY* list;
    NTSTATUS status;

    KeWaitForSingleObject(&gProtProcMutex, Executive, KernelMode, FALSE, NULL);

    list = gProtectedProcesses.Flink;

    while (list != &gProtectedProcesses)
    {
        PROT_PROC_INFO* pProtInfo = CONTAINING_RECORD(list, PROT_PROC_INFO, Link);

        list = list->Flink;

        RemoveEntryList(&pProtInfo->Link);

        ExFreePoolWithTag(pProtInfo, PROT_PROC_TAG);
    }

    if (bShouldObUnregister)
    {
        ObUnRegisterCallbacks(gRegistrationHandle);
        bShouldObUnregister = FALSE;
    }

    if (bShouldUnregisterThread)
    {
        status = PsRemoveCreateThreadNotifyRoutine(_Drv0ThreadCreateNotifyRoutine);
        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] PsRemoveCreateThreadNotifyRoutine failed: 0x%08x\n", status);
        }
        bShouldUnregisterThread = FALSE;
    }

    KeReleaseMutex(&gProtProcMutex, FALSE);

    return STATUS_SUCCESS;
}


NTSTATUS
Drv0ProtectProcess(
    _In_ IOCTL_PROT_UNPROT_PROC_STRUCT* ProtStruct
)
{
    PROT_PROC_INFO* pProtProc;
    BOOLEAN found = FALSE;
    LIST_ENTRY* list;

    KeWaitForSingleObject(&gProtProcMutex, Executive, KernelMode, FALSE, NULL);

    list = gProtectedProcesses.Flink;

    while (list != &gProtectedProcesses)
    {
        PROT_PROC_INFO* pProtInfo = CONTAINING_RECORD(list, PROT_PROC_INFO, Link);

        list = list->Flink;

        if (pProtInfo->Pid == ProtStruct->Pid)
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        KeReleaseMutex(&gProtProcMutex, FALSE);

        return STATUS_ALREADY_INITIALIZED;
    }

    Drv0LogInfo("[INFO] Protecting process with PID: %d\n", ProtStruct->Pid);

    pProtProc = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROT_PROC_INFO), PROT_PROC_TAG);
    if (pProtProc == NULL)
    {
        Drv0LogError("[ERROR] Failed to allocate prot info structure!");

        KeReleaseMutex(&gProtProcMutex, FALSE);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pProtProc->Pid = ProtStruct->Pid;

    InsertTailList(&gProtectedProcesses, &pProtProc->Link);

    KeReleaseMutex(&gProtProcMutex, FALSE);

    return STATUS_SUCCESS;
}


NTSTATUS
Drv0UnprotectProcess(
    _In_ IOCTL_PROT_UNPROT_PROC_STRUCT* ProtStruct
)
{
    LIST_ENTRY* list;
    BOOLEAN found = FALSE;

    KeWaitForSingleObject(&gProtProcMutex, Executive, KernelMode, FALSE, NULL);

    list = gProtectedProcesses.Flink;

    while (list != &gProtectedProcesses)
    {
        PROT_PROC_INFO* pProtInfo = CONTAINING_RECORD(list, PROT_PROC_INFO, Link);

        list = list->Flink;

        if (pProtInfo->Pid == ProtStruct->Pid)
        {
            RemoveEntryList(&pProtInfo->Link);

            ExFreePoolWithTag(pProtInfo, PROT_PROC_TAG);

            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        Drv0LogError("[ERROR] Process with PID %d not found among the protected ones!\n", ProtStruct->Pid);

        KeReleaseMutex(&gProtProcMutex, FALSE);

        return STATUS_NOT_FOUND;
    }

    Drv0LogInfo("[INFO] Unprotecting process with PID: %d\n", ProtStruct->Pid);
 
    KeReleaseMutex(&gProtProcMutex, FALSE);

    return STATUS_SUCCESS;
}