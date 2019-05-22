#include "ModuleFilter.h"
#include "Trace.h"
#include "ModuleFilter.tmh"


static VOID
ModLoadImageCallback(
    _In_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{
    WCHAR* wName;

    if (0 == ProcessId)
    {
        return;
    }

    if (FullImageName == NULL)
    {
        wName = u"<no name>";
    }
    else
    {
        wName = FullImageName->Buffer;
    }

    LogInfo("[MODULE] [MODULE-LOAD] Process %d, %S loaded at %llx, size %llx", (ULONG)(SIZE_T)ProcessId, wName, ImageInfo ? (SIZE_T)ImageInfo->ImageBase : 0, ImageInfo ? ImageInfo->ImageSize : 0);
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