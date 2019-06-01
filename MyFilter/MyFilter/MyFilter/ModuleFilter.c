#include "ModuleFilter.h"
#include "Trace.h"
#include "ModuleFilter.tmh"
#include "CommShared.h"

void
ModFltSendMessageModuleLoad(
    _In_ HANDLE ProcessId,
    _In_ PUNICODE_STRING ImagePath,
    _In_ ULONG Properties,
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize
)
{
    PMY_DRIVER_MSG_IMAGE_LOAD_NOTIFICATION pMsg = NULL;
    LARGE_INTEGER time = { 0 };

    ULONG msgSize = sizeof(MY_DRIVER_MSG_IMAGE_LOAD_NOTIFICATION);
    if (ImagePath)
    {
        msgSize += ImagePath->Length;
    }

    pMsg = ExAllocatePoolWithTag(PagedPool, msgSize, 'GSM+');
    if (!pMsg)
    {
        return;
    }

    KeQuerySystemTimePrecise(&time);

    pMsg->Header.TimeStamp = time.QuadPart;
    pMsg->Header.Result = STATUS_SUCCESS;
    pMsg->Header.MessageCode = msgModuleLoad;

    pMsg->ProcessId = HandleToULong(ProcessId);
    pMsg->Properties = Properties;
    pMsg->ImageSize = ImageSize;
    pMsg->ImageBase = ImageBase;

    if (ImagePath)
    {
        pMsg->ImagePathLength = ImagePath->Length;
        RtlCopyMemory(&pMsg->Data[0], ImagePath->Buffer, ImagePath->Length);
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
ModLoadImageCallback(
    _In_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{

    if (0 == ProcessId)
    {
        return;
    }

    //LogInfo("[MODULE] [MODULE-LOAD] Process %d, %S loaded at %llx, size %llx", (ULONG)(SIZE_T)ProcessId, wName, ImageInfo ? (SIZE_T)ImageInfo->ImageBase : 0, ImageInfo ? ImageInfo->ImageSize : 0);
    ModFltSendMessageModuleLoad(ProcessId, FullImageName, ImageInfo->Properties, ImageInfo->ImageBase, ImageInfo->ImageSize);
}


NTSTATUS
ModFltInitialize()
{
    NTSTATUS status;

    status = PsSetLoadImageNotifyRoutine(ModLoadImageCallback);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] PsSetLoadImageNotifyRoutine failed: 0x%08x\n", status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ModFltUninitialize()
{
    NTSTATUS status;

    status = PsRemoveLoadImageNotifyRoutine(ModLoadImageCallback);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] PsRemoveLoadImageNotifyRoutine failed: 0x%08x\n", status);
        return status;
    }

    return STATUS_SUCCESS;
}