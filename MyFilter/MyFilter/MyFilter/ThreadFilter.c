#include "ThreadFilter.h"
#include "Trace.h"
#include "ThreadFilter.tmh"
#include "CommShared.h"

static void
ThrFltSendMessageThrCreate(
    HANDLE ProcessId,
    HANDLE ThreadId
)
{
    PMY_DRIVER_MSG_THREAD_CREATE_NOTIFICATION msg = NULL;
    LARGE_INTEGER time = { 0 };

    msg = ExAllocatePoolWithTag(PagedPool, sizeof(*msg), 'GSM+');
    if (!msg)
    {
        return;
    }

    KeQuerySystemTimePrecise(&time);

    msg->Header.TimeStamp = time.QuadPart;
    msg->Header.Result = STATUS_SUCCESS;
    msg->Header.MessageCode = msgThreadCreate;

    msg->ProcessId = HandleToULong(ProcessId);
    msg->ThreadId = HandleToULong(ThreadId);

    NTSTATUS status = CommSendMessageOnThreadPool(msg, sizeof(*msg), NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }
}


static void
ThrFltSendMessageThrTerminate(
    HANDLE ProcessId,
    HANDLE ThreadId
)
{
    PMY_DRIVER_MSG_THREAD_CREATE_NOTIFICATION msg = NULL;
    LARGE_INTEGER time = { 0 };

    msg = ExAllocatePoolWithTag(PagedPool, sizeof(*msg), 'GSM+');
    if (!msg)
    {
        return;
    }

    KeQuerySystemTimePrecise(&time);

    msg->Header.TimeStamp = time.QuadPart;
    msg->Header.Result = STATUS_SUCCESS;
    msg->Header.MessageCode = msgThreadTerminate;

    msg->ProcessId = HandleToULong(ProcessId);
    msg->ThreadId = HandleToULong(ThreadId);

    NTSTATUS status = CommSendMessageOnThreadPool(msg, sizeof(*msg), NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }
}

static void
ThrThreadCreateNotifyRoutine(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ BOOLEAN Create
)
{
    if (Create)
    {
        //LogInfo("[THREAD] [CREATE] Pid %d Tid: %d", (ULONG)(SIZE_T)ProcessId, (ULONG)(SIZE_T)ThreadId);
        ThrFltSendMessageThrCreate(ProcessId, ThreadId);
    }
    else
    {
        //LogInfo("[THREAD] [TERMINATE] Pid %d Tid: %d", (ULONG)(SIZE_T)ProcessId, (ULONG)(SIZE_T)ThreadId);
        ThrFltSendMessageThrTerminate(ProcessId, ThreadId);
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