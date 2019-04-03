#include "notifications.h"

#include "trace.h"

#include "./x64/Debug/notifications.tmh"

BOOLEAN gProcessCallbackInitialized = FALSE;
BOOLEAN gDriverCallbackInitialized = FALSE;
BOOLEAN gRegisterCallbackInitialized = FALSE;

LARGE_INTEGER gCookie;
UNICODE_STRING gAltitude;


static VOID
_Drv0CreateRemoveProcessCallback(
    _In_ PEPROCESS Process,
    _In_ HANDLE ProcessId,
    _In_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
    DWORD pid = (DWORD)(SIZE_T)ProcessId;

    if (NULL == CreateInfo)
    {
        Drv0LogInfo("[PROCESS] Process %p, pid %d removed!\n", Process, pid);
    }
    else
    {
        Drv0LogInfo("[PROCESS] Process %p, pid %d created!\n", Process, pid);
        Drv0LogInfo("[PROCESS] Name: %wZ, command line: %wZ", CreateInfo->ImageFileName, CreateInfo->CommandLine);
    }
}


static VOID
_Drv0LoadImageCallback(
    _In_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
)
{
    WCHAR* wName;

    if (0 != ProcessId)
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

    Drv0LogInfo("[DRIVER] Driver %S loaded at %llx, size %llx", wName, ImageInfo ? (SIZE_T)ImageInfo->ImageBase : 0, ImageInfo ? ImageInfo->ImageSize : 0);
}


static NTSTATUS
_Drv0RegExCallbackFunction(
    PVOID CallbackContext,
    PVOID Argument1,
    PVOID Argument2
)
{
    REG_NOTIFY_CLASS notifyClass = (REG_NOTIFY_CLASS)((SIZE_T)Argument1);
    NTSTATUS status;

    UNREFERENCED_PARAMETER(CallbackContext);

    if (notifyClass == RegNtRenameKey)
    {
        REG_RENAME_KEY_INFORMATION *pInfo = (REG_RENAME_KEY_INFORMATION*)Argument2;
        UNICODE_STRING *objName;

        status = CmCallbackGetKeyObjectID(
            &gCookie,
            pInfo->Object,
            NULL,
            &objName);
        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] CmCallbackGetKeyObjectID failed: 0x%08x\n", status);
            goto _exit;
        }

        Drv0LogInfo("[REGISTRY] Old name: %wZ, new name: %wZ\n", objName, pInfo->NewName);
    }

_exit:
    return STATUS_SUCCESS;
}



NTSTATUS
Drv0InitializeNotifications(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS status;
    status = PsSetCreateProcessNotifyRoutineEx(
        _Drv0CreateRemoveProcessCallback,
        FALSE
    );

    if (!NT_SUCCESS(status))
    {
        Drv0LogError("[ERROR] PsSetCreateProcessNotifyRoutineEx failed: 0x%08x\n", status);
        return status;
    }

    gProcessCallbackInitialized = TRUE;
    
    status = PsSetLoadImageNotifyRoutine(_Drv0LoadImageCallback);
    if (!NT_SUCCESS(status))
    {
        Drv0LogError("[ERROR] PsSetLoadImageNotifyRoutine failed: 0x%08x\n", status);
        return status;
    }

    gDriverCallbackInitialized = TRUE;

    {
        RtlInitUnicodeString(&gAltitude, u"320000");

        status = CmRegisterCallbackEx(
            _Drv0RegExCallbackFunction,
            &gAltitude,
            DriverObject,
            NULL,
            &gCookie,
            NULL
        );

        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] CmRegisterCallbackEx failed: 0x%08x\n", status);
            return status;
        }
    }

    gRegisterCallbackInitialized = TRUE;

    return STATUS_SUCCESS;


}

NTSTATUS
Drv0UninitializeNotifications(
    VOID
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (gProcessCallbackInitialized)
    {
        status = PsSetCreateProcessNotifyRoutineEx(
            _Drv0CreateRemoveProcessCallback,
            TRUE
        );
        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] PsSetCreateProcessNotifyRoutineEx failed: 0x%08x\n", status);
        }

        gProcessCallbackInitialized = FALSE;
    }

    if (gDriverCallbackInitialized)
    {
        status = PsRemoveLoadImageNotifyRoutine(_Drv0LoadImageCallback);
        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] PsRemoveLoadImageNotifyRoutine failed: 0x%08x\n", status);
        }

        gDriverCallbackInitialized = FALSE;
    }

    if (gRegisterCallbackInitialized)
    {
        status = CmUnRegisterCallback(gCookie);
        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] CmUnRegisterCallback failed: 0x%08x\n", status);
        }

        gRegisterCallbackInitialized = FALSE;
    }

    return STATUS_SUCCESS;
}