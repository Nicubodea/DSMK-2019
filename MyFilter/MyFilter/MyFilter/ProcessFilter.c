//
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#include "ProcessFilter.h"
#include "Communication.h"
#include "CommShared.h"
#include "Trace.h"
#include "ProcessFilter.tmh"

void
ProcFltSendMessageProcessCreate(
    _In_ HANDLE ProcessId,
    _In_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    PMY_DRIVER_MSG_PROCESS_NOTIFICATION pMsg = NULL;
    MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY reply = {0};

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

    ULONG replySize = sizeof(reply);
    NTSTATUS status = CommSendMessage(pMsg, msgSize, &reply, &replySize);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }

    ExFreePoolWithTag(pMsg, 'GSM+');
}

static VOID
ProcFltSendMessageProcessTerminate(
    _In_ HANDLE ProcessId
)
{
    MY_DRIVER_PROCESS_TERMINATE_MSG msg;
    msg.Header.MessageCode = msgProcessTerminate;

    msg.ProcessId = HandleToULong(ProcessId);

    ULONG replySize = 0;
    NTSTATUS status = CommSendMessage(&msg, sizeof(msg), NULL, &replySize);
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
    return PsSetCreateProcessNotifyRoutineEx(ProcFltNotifyRoutine, FALSE);
}

NTSTATUS
ProcFltUninitialize()
{
    return PsSetCreateProcessNotifyRoutineEx(ProcFltNotifyRoutine, TRUE);
}
