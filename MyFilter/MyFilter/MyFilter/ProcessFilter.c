//
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#include "ProcessFilter.h"
#include "Communication.h"
#include "CommShared.h"
#include "Trace.h"
#include "ProcessFilter.tmh"
#include "Options.h"

void
ProcFltSendMessageProcessCreate(
    _In_ HANDLE ProcessId,
    _In_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    PMY_DRIVER_MSG_PROCESS_NOTIFICATION pMsg = NULL;
    LARGE_INTEGER time = { 0 };

    ULONG msgSize = sizeof(MY_DRIVER_MSG_PROCESS_NOTIFICATION);
    if (CreateInfo->ImageFileName)
    {
        msgSize += CreateInfo->ImageFileName->Length;
    }

    pMsg = ExAllocatePoolWithTag(PagedPool, msgSize, 'GSM+');
    if (!pMsg)
    {
        return;
    }

    KeQuerySystemTimePrecise(&time);

    pMsg->Header.TimeStamp = time.QuadPart;
    pMsg->Header.Result = STATUS_SUCCESS;
    pMsg->Header.MessageCode = msgProcessCreate;

    pMsg->ProcessId = HandleToULong(ProcessId);

    if (CreateInfo->ImageFileName)
    {
        pMsg->ImagePathLength = CreateInfo->ImageFileName->Length;
        RtlCopyMemory(&pMsg->Data[0], CreateInfo->ImageFileName->Buffer, CreateInfo->ImageFileName->Length);
    }
    else
    {
        pMsg->ImagePathLength = 0;
    }

    NTSTATUS status = CommSendMessageOnThreadPool(pMsg, msgSize, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }

    //ExFreePoolWithTag(pMsg, 'GSM+');
}

static VOID
ProcFltSendMessageProcessTerminate(
    _In_ HANDLE ProcessId
)
{
    PMY_DRIVER_PROCESS_TERMINATE_MSG pMsg = NULL;
    LARGE_INTEGER time = { 0 };

    ULONG msgSize = sizeof(MY_DRIVER_PROCESS_TERMINATE_MSG);

    pMsg = ExAllocatePoolWithTag(PagedPool, msgSize, 'GSM+');
    if (!pMsg)
    {
        return;
    }

    KeQuerySystemTimePrecise(&time);

    pMsg->Header.TimeStamp = time.QuadPart;
    pMsg->Header.Result = STATUS_SUCCESS;
    pMsg->Header.MessageCode = msgProcessTerminate;

    pMsg->ProcessId = HandleToULong(ProcessId);

    NTSTATUS status = CommSendMessageOnThreadPool(pMsg, sizeof(*pMsg), NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }
}

static VOID
ProcFltNotifyRoutine(
    _Inout_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    UNREFERENCED_PARAMETER(Process);

    if (CreateInfo)
    {
        ProcFltSendMessageProcessCreate(ProcessId, CreateInfo);
    }
    else
    {
        ProcFltSendMessageProcessTerminate(ProcessId);
    }
}

NTSTATUS
ProcFltInitialize()
{
    LogInfo("Initializing Process Filtering!");
    return PsSetCreateProcessNotifyRoutineEx(ProcFltNotifyRoutine, FALSE);
}

NTSTATUS
ProcFltUninitialize()
{
    LogInfo("Uninitializing Process Filtering!");
    return PsSetCreateProcessNotifyRoutineEx(ProcFltNotifyRoutine, TRUE);
}
