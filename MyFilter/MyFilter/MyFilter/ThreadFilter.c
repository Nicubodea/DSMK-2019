#include "ThreadFilter.h"
#include "Trace.h"
#include "ThreadFilter.tmh"


static void
ThrThreadCreateNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    if (Create)
    {
        LogInfo("[THREAD] [CREATE] Pid %d Tid: %d", (ULONG)(SIZE_T)ProcessId, (ULONG)(SIZE_T)ThreadId);
    }
    else
    {
        LogInfo("[THREAD] [TERMINATE] Pid %d Tid: %d", (ULONG)(SIZE_T)ProcessId, (ULONG)(SIZE_T)ThreadId);
    }
}


NTSTATUS
ThrFltInitialize()
{
    NTSTATUS status;

    status = PsSetCreateThreadNotifyRoutine(ThrThreadCreateNotifyRoutine);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] PsSetCreateThreadNotifyRoutine failed: 0x%08x\n", status);
        return status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ThrFltUninitialize()
{
    NTSTATUS status;

    status = PsRemoveCreateThreadNotifyRoutine(ThrThreadCreateNotifyRoutine);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] PsRemoveCreateThreadNotifyRoutine failed: 0x%08x\n", status);
        return status;
    }
    
    return STATUS_SUCCESS;
}