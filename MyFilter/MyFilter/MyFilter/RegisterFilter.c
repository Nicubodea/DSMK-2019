#include "RegisterFilter.h"
#include "Trace.h"
#include "RegisterFilter.tmh"

LARGE_INTEGER gCookie;
UNICODE_STRING gAltitude;

static NTSTATUS
RegExCallbackFunction(
    PVOID CallbackContext,
    PVOID Argument1,
    PVOID Argument2
)
{
    REG_NOTIFY_CLASS notifyClass = (REG_NOTIFY_CLASS)((SIZE_T)Argument1);
    NTSTATUS status;

    UNREFERENCED_PARAMETER(CallbackContext);

    if (notifyClass == RegNtPostCreateKeyEx)
    {
        REG_POST_OPERATION_INFORMATION *pInfo = (REG_POST_OPERATION_INFORMATION *)Argument2;

        UNICODE_STRING *objName;

        if (pInfo->Status != STATUS_SUCCESS)
        {
            goto _exit;
        }

        status = CmCallbackGetKeyObjectIDEx(
            &gCookie,
            pInfo->Object,
            NULL,
            &objName,
            0);
        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] CmCallbackGetKeyObjectIDEx failed: 0x%08x\n", status);
            goto _exit;
        }

        LogInfo("[REGISTRY] [CREATE] Key: %wZ\n", objName);
    }
    if (notifyClass == RegNtPreSetValueKey)
    {
        REG_SET_VALUE_KEY_INFORMATION *pInfo = (REG_SET_VALUE_KEY_INFORMATION *)Argument2;

        UNICODE_STRING *objName;

        status = CmCallbackGetKeyObjectIDEx(
            &gCookie,
            pInfo->Object,
            NULL,
            &objName,
            0);
        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] CmCallbackGetKeyObjectIDEx failed: 0x%08x\n", status);
            goto _exit;
        }

        LogInfo("[REGISTRY] [SET-VALUE] Key: %wZ, Value: %wZ", objName, pInfo->ValueName);
    }

    if (notifyClass == RegNtDeleteKey)
    {
        REG_DELETE_KEY_INFORMATION *pInfo = (REG_DELETE_KEY_INFORMATION *)Argument2;

        UNICODE_STRING *objName;

        status = CmCallbackGetKeyObjectIDEx(
            &gCookie,
            pInfo->Object,
            NULL,
            &objName,
            0);
        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] CmCallbackGetKeyObjectIDEx failed: 0x%08x\n", status);
            goto _exit;
        }

        LogInfo("[REGISTRY] [DELETE-KEY] Key: %wZ", objName);
    }

    if (notifyClass == RegNtDeleteValueKey)
    {
        REG_DELETE_VALUE_KEY_INFORMATION *pInfo = (REG_DELETE_VALUE_KEY_INFORMATION *)Argument2;

        UNICODE_STRING *objName;

        status = CmCallbackGetKeyObjectIDEx(
            &gCookie,
            pInfo->Object,
            NULL,
            &objName,
            0);
        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] CmCallbackGetKeyObjectIDEx failed: 0x%08x\n", status);
            goto _exit;
        }
        
        LogInfo("[REGISTRY] [DELETE-VALUE] Key: %wZ, Value: %wZ", objName, pInfo->ValueName);
    }

    if (notifyClass == RegNtPreLoadKey)
    {
        REG_LOAD_KEY_INFORMATION *pInfo = (REG_LOAD_KEY_INFORMATION *)Argument2;

        LogInfo("[REGISTRY] [LOAD-KEY] %wZ", pInfo->KeyName);
    }

    if (notifyClass == RegNtRenameKey)
    {
        REG_RENAME_KEY_INFORMATION *pInfo = (REG_RENAME_KEY_INFORMATION*)Argument2;

        UNICODE_STRING *objName;

        status = CmCallbackGetKeyObjectIDEx(
            &gCookie,
            pInfo->Object,
            NULL,
            &objName,
            0);
        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] CmCallbackGetKeyObjectIDEx failed: 0x%08x\n", status);
            goto _exit;
        }

        LogInfo("[REGISTRY] [RENAME] Old name: %wZ, new name: %wZ\n", objName, pInfo->NewName);
    }

_exit:
    return STATUS_SUCCESS;
}



NTSTATUS
RegFltInitialize(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    NTSTATUS status;
    RtlInitUnicodeString(&gAltitude, u"320000");

    status = CmRegisterCallbackEx(
        RegExCallbackFunction,
        &gAltitude,
        DriverObject,
        NULL,
        &gCookie,
        NULL
    );
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] CmRegisterCallbackEx failed: 0x%08x\n", status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RegFltUninitialize()
{
    NTSTATUS status;

    status = CmUnRegisterCallback(gCookie);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] CmUnRegisterCallback failed: 0x%08x\n", status);
        return status;
    }

    return STATUS_SUCCESS;
}