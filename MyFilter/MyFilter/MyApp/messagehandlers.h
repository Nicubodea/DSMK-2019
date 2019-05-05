#ifndef _MESSAGEHANDLERS_H_
#define _MESSAGEHANDLERS_H_
//
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//

/*++
    This header file contains handlers for all the messages that can be received by MYAPP from DRIVER.
--*/

#define WIN32_NO_STATUS
#include <windows.h>
#undef  WIN32_NO_STATUS
#include <ntstatus.h>
#include <fltUser.h>
#include "globaldata.h"

typedef 
_Function_class_(FUNC_MessageHandler)
NTSTATUS
FUNC_MessageHandler(
    _In_bytecount_(InputBufferSize) PFILTER_MESSAGE_HEADER InputBuffer,
    _In_ DWORD InputBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesWritten) PFILTER_REPLY_HEADER OutputBuffer,
    _In_ DWORD OutputBufferSize,
    _Out_ PDWORD BytesWritten
    );
typedef FUNC_MessageHandler *PFUNC_MessageHandler;

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
    );

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
    );

#endif//_MESSAGEHANDLERS_H_