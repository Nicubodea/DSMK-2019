//
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//

#include "messagehandlers.h"
#include <ntstatus.h>
#include "globaldata.h"
#include "CommShared.h"
#include <malloc.h>

CHAR* messageCodeToString[] = {
    "msgProcessCreate",
    "msgProcessTerminate",
    "msgThreadCreate",
    "msgThreadTerminate",
    "msgModuleLoad",
    "msgRegistryOp",
    "msgFileOp",
};

CHAR* regTypeToString[] = {
    "regKeyCreate",
    "regValueSet",
    "regKeyDelete",
    "regValueDelete",
    "regKeyLoad",
    "regKeyRename"
};

CHAR* fileTypeToString[] = {
    "fileCreate",
    "fileClose",
    "fileCleanup",
    "fileRead",
    "fileWrite",
    "fileSetAttributes"
};

//
// MsgHandleUnknownMessage
//
_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleUnknownMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(InputBufferSize);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    PMY_DRIVER_MESSAGE_HEADER pHeader = (PMY_DRIVER_MESSAGE_HEADER)(InputBuffer + 1);

    wprintf(L"[Error] Unknown message received form driver. Id = %u", pHeader->MessageCode);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}

//
// MsgHandleProcessCreate
//
_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleProcessCreate(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    PMY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE pInput = (PMY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE)InputBuffer;
    BOOL bShouldCleanupPath = FALSE;
    ULONGLONG timestamp = pInput->Message.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Message.Header.MessageCode];
    NTSTATUS result = pInput->Message.Header.Result;
    PWCHAR path;

    UNREFERENCED_PARAMETER(OutputBufferSize);
    UNREFERENCED_PARAMETER(OutputBuffer);

    *BytesWritten = 0;
    if (InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (sizeof(*pInput) + pInput->Message.ImagePathLength < sizeof(*pInput))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    if (InputBufferSize < sizeof(*pInput) + pInput->Message.ImagePathLength)
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (!pInput->Message.ImagePathLength)
    {
        path = u"NULL";
    }
    else
    {
        path = malloc(pInput->Message.ImagePathLength + sizeof(WCHAR));
        if (!path)
        {
            path = u"BAD_ALLOC";
        }
        else
        {
            memcpy(path, &pInput->Message.Data[0], pInput->Message.ImagePathLength);
            path[pInput->Message.ImagePathLength >> 1] = L'\0';
            bShouldCleanupPath = TRUE;
        }
    }

    wprintf(L"[%lld][%S][Result %08x][Pid = %d][path = %s]\n", timestamp, operation, result, pInput->Message.ProcessId, path);

    if (bShouldCleanupPath)
    {
        free(path);
    }

    return STATUS_SUCCESS;
}

_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleProcessTerminate(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_  DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_  DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferSize);

    PMY_DRIVER_PROCESS_TERMINATE_MSG_FULL pInput = (PMY_DRIVER_PROCESS_TERMINATE_MSG_FULL)InputBuffer;
    ULONGLONG timestamp = pInput->Msg.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Msg.Header.MessageCode];
    NTSTATUS result = pInput->Msg.Header.Result;

    if (!InputBuffer || InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    wprintf(L"[%lld][%S][Result %08x][Pid = %d]\n", timestamp, operation, result, pInput->Msg.ProcessId);

    *BytesWritten = 0;
	return STATUS_SUCCESS;
}


_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleThreadCreate(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_  DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_  DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    PMY_DRIVER_MSG_THREAD_CREATE_MSG_FULL pInput = (PMY_DRIVER_MSG_THREAD_CREATE_MSG_FULL)InputBuffer;
    ULONGLONG timestamp = pInput->Msg.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Msg.Header.MessageCode];
    NTSTATUS result = pInput->Msg.Header.Result;

    if (!InputBuffer || InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    wprintf(L"[%lld][%S][Result %08x][Pid = %d][Tid = %d]\n", timestamp, operation, result, pInput->Msg.ProcessId, pInput->Msg.ThreadId);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}


_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleThreadTerminate(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_  DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_  DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    PMY_DRIVER_MSG_THREAD_TERMINATE_MSG_FULL pInput = (PMY_DRIVER_MSG_THREAD_TERMINATE_MSG_FULL)InputBuffer;
    ULONGLONG timestamp = pInput->Msg.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Msg.Header.MessageCode];
    NTSTATUS result = pInput->Msg.Header.Result;

    if (!InputBuffer || InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    wprintf(L"[%lld][%S][Result %08x][Pid = %d][Tid = %d]\n", timestamp, operation, result, pInput->Msg.ProcessId, pInput->Msg.ThreadId);

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}


_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleModuleLoad(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_  DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_  DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    PMY_DRIVER_MSG_IMAGE_LOAD_MSG_FULL pInput = (PMY_DRIVER_MSG_IMAGE_LOAD_MSG_FULL)InputBuffer;
    ULONGLONG timestamp = pInput->Msg.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Msg.Header.MessageCode];
    NTSTATUS result = pInput->Msg.Header.Result;
    PWCHAR path;
    BOOLEAN bShouldCleanupPath = FALSE;

    if (!InputBuffer || InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (!pInput->Msg.ImagePathLength)
    {
        path = u"NULL";
    }
    else
    {
        path = malloc(pInput->Msg.ImagePathLength + sizeof(WCHAR));
        if (!path)
        {
            path = u"BAD_ALLOC";
        }
        else
        {
            memcpy(path, &pInput->Msg.Data[0], pInput->Msg.ImagePathLength);
            path[pInput->Msg.ImagePathLength >> 1] = L'\0';
            bShouldCleanupPath = TRUE;
        }
    }
    
    wprintf(L"[%lld][%S][Result %08x][pid = %d][path = %s][properties = %d][base = %p][size = %zx]\n", timestamp, operation, result, 
        pInput->Msg.ProcessId, path,
        pInput->Msg.Properties, pInput->Msg.ImageBase, pInput->Msg.ImageSize);

    if (bShouldCleanupPath)
    {
        free(path);
    }

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}


_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleFileOperation(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_  DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_  DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    PMY_DRIVER_MSG_FILE_MSG_FULL pInput = (PMY_DRIVER_MSG_FILE_MSG_FULL)InputBuffer;
    ULONGLONG timestamp = pInput->Msg.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Msg.Header.MessageCode];
    NTSTATUS result = pInput->Msg.Header.Result;
    PWCHAR path;
    BOOLEAN bShouldCleanupPath = FALSE;
    CHAR *type;

    if (!InputBuffer || InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (!pInput->Msg.NameLen)
    {
        path = u"NULL";
    }
    else
    {
        path = malloc(pInput->Msg.NameLen + sizeof(WCHAR));
        if (!path)
        {
            path = u"BAD_ALLOC";
        }
        else
        {
            memcpy(path, &pInput->Msg.Data[0], pInput->Msg.NameLen);
            path[pInput->Msg.NameLen >> 1] = L'\0';
            bShouldCleanupPath = TRUE;
        }
    }

    type = fileTypeToString[pInput->Msg.Type];

    wprintf(L"[%lld][%S][Result %08x][Type = %S][path = %s]\n", timestamp, operation, result, type, path);

    if (bShouldCleanupPath)
    {
        free(path);
    }

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}


_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgHandleRegistryOperation(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_  DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_  DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
)
{
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    PMY_DRIVER_MSG_REGISTRY_MSG_FULL pInput = (PMY_DRIVER_MSG_REGISTRY_MSG_FULL)InputBuffer;
    ULONGLONG timestamp = pInput->Msg.Header.TimeStamp;
    CHAR* operation = messageCodeToString[pInput->Msg.Header.MessageCode];
    NTSTATUS result = pInput->Msg.Header.Result;
    PWCHAR path;
    PWCHAR pathaux;
    BOOLEAN bShouldCleanupPath = FALSE, bShouldCleanupPathAux = FALSE;
    CHAR *type;

    if (!InputBuffer || InputBufferSize < sizeof(*pInput))
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (!pInput->Msg.NameLen)
    {
        path = u"NULL";
    }
    else
    {
        path = malloc(pInput->Msg.NameLen + sizeof(WCHAR));
        if (!path)
        {
            path = u"BAD_ALLOC";
        }
        else
        {
            memcpy(path, &pInput->Msg.Data[0], pInput->Msg.NameLen);
            path[pInput->Msg.NameLen >> 1] = L'\0';
            bShouldCleanupPath = TRUE;
        }
    }

    if (!pInput->Msg.NewNameLen)
    {
        pathaux = u"NULL";
    }
    else
    {
        pathaux = malloc(pInput->Msg.NewNameLen + sizeof(WCHAR));
        if (!pathaux)
        {
            pathaux = u"BAD_ALLOC";
        }
        else
        {
            memcpy(pathaux, &pInput->Msg.Data[0], pInput->Msg.NewNameLen);
            pathaux[pInput->Msg.NewNameLen >> 1] = L'\0';
            bShouldCleanupPathAux = TRUE;
        }
    }

    type = fileTypeToString[pInput->Msg.Type];

    wprintf(L"[%lld][%S][Result %08x][Type = %S][path = %s]", timestamp, operation, result, type, path);

    if (pInput->Msg.Type == regKeyRename)
    {
        wprintf(L"[new key = %s]", pathaux);
    }
    if (pInput->Msg.Type == regValueDelete)
    {
        wprintf(L"[deleted value = %s]", pathaux);
    }
    if (pInput->Msg.Type == regValueSet)
    {
        wprintf(L"[set value = %s]", pathaux);
    }
    wprintf(L"\n");

    if (bShouldCleanupPath)
    {
        free(path);
    }

    if (bShouldCleanupPathAux)
    {
        free(path);
    }

    *BytesWritten = 0;
    return STATUS_SUCCESS;
}


//
// MsgDispatchNewMessage
//
_Pre_satisfies_(InputBufferSize >= sizeof(FILTER_MESSAGE_HEADER))
_Pre_satisfies_(OutputBufferSize >= sizeof(FILTER_REPLY_HEADER))
NTSTATUS
MsgDispatchNewMessage(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    if (InputBufferSize < sizeof(FILTER_MESSAGE_HEADER) + sizeof(MY_DRIVER_COMMAND_HEADER))
    {
        wprintf(L"[Error] Message size is too small to dispatch. Size = %d\n", InputBufferSize);
        return STATUS_BUFFER_TOO_SMALL;
    }

    PMY_DRIVER_MESSAGE_HEADER pHeader = (PMY_DRIVER_MESSAGE_HEADER)(InputBuffer + 1);

    switch (pHeader->MessageCode)
    {
    case msgProcessCreate:
        status = MsgHandleProcessCreate(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    case msgProcessTerminate:
        status = MsgHandleProcessTerminate(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    case msgThreadCreate:
        status = MsgHandleThreadCreate(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    case msgThreadTerminate:
        status = MsgHandleThreadTerminate(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    case msgModuleLoad:
        status = MsgHandleModuleLoad(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    case msgRegistryOp:
        status = MsgHandleRegistryOperation(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    case msgFileOp:
        status = MsgHandleFileOperation(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    default:
        status =  MsgHandleUnknownMessage(InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, BytesWritten);
        break;
    }
    
    return status;
}
