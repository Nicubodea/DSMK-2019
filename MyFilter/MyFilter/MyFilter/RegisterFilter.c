#include "RegisterFilter.h"
#include "Trace.h"
#include "RegisterFilter.tmh"
#include "CommShared.h"

LARGE_INTEGER gCookie;
UNICODE_STRING gAltitude;

void
RegFltSendMessageRegistry(
    _In_ PUNICODE_STRING Name,
    _In_opt_ PUNICODE_STRING NewName,
    _In_ MY_DRIVER_MSG_KEY_TYPE Type
)
{
    PMY_DRIVER_MSG_REGISTRY_NOTIFICATION pMsg = NULL;
    LARGE_INTEGER time = { 0 };
    ULONG lastData = 0;

    ULONG msgSize = sizeof(MY_DRIVER_MSG_REGISTRY_NOTIFICATION);
    if (Name)
    {
        msgSize += Name->Length;
    }

    if (NewName)
    {
        msgSize += NewName->Length;
    }

    pMsg = ExAllocatePoolWithTag(PagedPool, msgSize, 'GSM+');
    if (!pMsg)
    {
        return;
    }

    KeQuerySystemTimePrecise(&time);

    pMsg->Header.TimeStamp = time.QuadPart;
    pMsg->Header.Result = STATUS_SUCCESS;
    pMsg->Header.MessageCode = msgRegistryOp;

    pMsg->Type = Type;

    if (Name)
    {
        pMsg->NameLen = Name->Length;
        RtlCopyMemory(&pMsg->Data[0], Name->Buffer, Name->Length);
        lastData = pMsg->NameLen;
    }
    else
    {
        pMsg->NameLen = 0;
    }

    if (NewName)
    {
        pMsg->NewNameLen = NewName->Length;
        RtlCopyMemory(&pMsg->Data[lastData], NewName->Buffer, NewName->Length);
    }
    else
    {
        pMsg->NewNameLen = 0;
    }
    NTSTATUS status = CommSendMessageOnThreadPool(pMsg, msgSize, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }

    ExFreePoolWithTag(pMsg, 'GSM+');
}

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

        RegFltSendMessageRegistry(objName, NULL, regKeyCreate);
        //LogInfo("[REGISTRY] [CREATE] Key: %wZ\n", objName);
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

        RegFltSendMessageRegistry(objName, pInfo->ValueName, regValueSet);
        //LogInfo("[REGISTRY] [SET-VALUE] Key: %wZ, Value: %wZ", objName, pInfo->ValueName);
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

        RegFltSendMessageRegistry(objName, NULL, regKeyDelete);
        //LogInfo("[REGISTRY] [DELETE-KEY] Key: %wZ", objName);
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
        
        RegFltSendMessageRegistry(objName, pInfo->ValueName, regValueDelete);
        //LogInfo("[REGISTRY] [DELETE-VALUE] Key: %wZ, Value: %wZ", objName, pInfo->ValueName);
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

        RegFltSendMessageRegistry(objName, pInfo->NewName, regKeyRename);
        //LogInfo("[REGISTRY] [RENAME] Old name: %wZ, new name: %wZ\n", objName, pInfo->NewName);
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