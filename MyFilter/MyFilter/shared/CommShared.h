#ifndef _COMM_SHARED_HPP_INCLUDED_
#define _COMM_SHARED_HPP_INCLUDED_
//

//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//

#ifdef USER_MODE
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <fltUser.h>
#include "globaldata.h"
#else
#include <fltKernel.h>
#endif

#define MY_FILTER_PORT_NAME L"\\MY_DRIVERCommPort"

//
// FLT_PORT_CONNECTION_CONTEXT - connection context used by MY_DRIVER
//
typedef struct _FLT_PORT_CONNECTION_CONTEXT
{
    ULONG Version;
}FLT_PORT_CONNECTION_CONTEXT, *PFLT_PORT_CONNECTION_CONTEXT;


#pragma region Commands
/*++
This region contains definitions for all the commands that can be received by MY_DRIVER driver form MY_DRIVERCORE trough the FLT port
--*/

//
// MY_DRIVER_COMMAND_CODE
//
typedef enum _MY_DRIVER_COMMAND_CODE
{
    commGetVersion = 1,
    commStartMonitoring = 2,
    commStopMonitoring = 3,
    /// When adding new new commands please change _Field_range_ for MY_DRIVER_COMMAND_HEADER
}MY_DRIVER_COMMAND_CODE, *PMY_DRIVER_COMMAND_CODE;

#pragma pack (push, 1)
//
// MY_DRIVER_COMMAND_HEADER
//
typedef struct _MY_DRIVER_COMMAND_HEADER
{
    _Field_range_(commGetVersion, commStopMonitoring) UINT32 CommandCode;
}MY_DRIVER_COMMAND_HEADER, *PMY_DRIVER_COMMAND_HEADER;

/// All commands must start with the MY_DRIVER_COMMAND_HEADER structure

//
// COMM_CMD_GET_VERSION
//
typedef struct _COMM_CMD_GET_VERSION
{
    MY_DRIVER_COMMAND_HEADER Header;
    ULONG Version;
}COMM_CMD_GET_VERSION, *PCOMM_CMD_GET_VERSION;
//
// COMM_CMD_START_MONITORING
//
typedef struct _COMM_CMD_START_MONITORING
{
    MY_DRIVER_COMMAND_HEADER Header;

}COMM_CMD_START_MONITORING, *PCOMM_CMD_START_MONITORING;

//
// COMM_CMD_STOP_MONITORING
//
typedef struct _COMM_CMD_STOP_MONITORING
{
    MY_DRIVER_COMMAND_HEADER Header;
}COMM_CMD_STOP_MONITORING, *PCOMM_CMD_STOP_MONITORING;

#pragma pack (pop)
#pragma endregion Commands

#pragma region Messages
/*++
   This region contains definitions for all the messages that can be received by MY_DRIVERCORE from the MY_DRIVER driver
--*/
typedef enum _MY_DRIVER_MESSAGE_CODE
{
    msgProcessCreate = 0,
    msgProcessTerminate = 1,
    msgMaxValue,
} MY_DRIVER_MESSAGE_CODE, *PMY_DRIVER_MESSAGE_CODE;

/// All messages must start with FILTER_MESSAGE_HEADER
/// All replies must start with  FILTER_REPLY_HEADER

#pragma pack(push, 1)
//
// MY_DRIVER_MESSAGE_HEADER
//
typedef struct _MY_DRIVER_MESSAGE_HEADER
{
     MY_DRIVER_MESSAGE_CODE MessageCode;
} MY_DRIVER_MESSAGE_HEADER, *PMY_DRIVER_MESSAGE_HEADER;

//
// MY_DRIVER_MSG_PROCESS_NOTIFICATION
//
typedef struct _MY_DRIVER_MSG_PROCESS_NOTIFICATION
{
    MY_DRIVER_MESSAGE_HEADER Header;
    UINT32 ProcessId;           // the process id
    USHORT ImagePathLength;     // size in bytes of the path
    UCHAR  Data[0];
} MY_DRIVER_MSG_PROCESS_NOTIFICATION, *PMY_DRIVER_MSG_PROCESS_NOTIFICATION;

//
// MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY
//
typedef struct _MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY
{
    NTSTATUS Status;
} MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY, *PMY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY;

//
// MY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE
//
typedef struct _MY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE
{
    FILTER_MESSAGE_HEADER Header;
    MY_DRIVER_MSG_PROCESS_NOTIFICATION Message;
} MY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE, *PMY_DRIVER_PROCESS_NOTIFICATION_FULL_MESSAGE;

//
// MY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY
//
typedef struct _MY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY
{
    FILTER_REPLY_HEADER Header;
    MY_DRIVER_PROCESS_CREATE_MESSAGE_REPLY Reply;
}MY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY, *PMY_DRIVER_PROCESS_CREATE_FULL_MESSAGE_REPLY;

typedef struct _MY_DRIVER_PROCESS_TERMINATE_MSG
{
    MY_DRIVER_MESSAGE_HEADER Header;
    ULONG32 ProcessId;
}MY_DRIVER_PROCESS_TERMINATE_MSG, *PMY_DRIVER_PROCESS_TERMINATE_MSG;

typedef struct _MY_DRIVER_PROCESS_TERMINATE_MSG_FULL
{
    FILTER_MESSAGE_HEADER Header;
    MY_DRIVER_PROCESS_TERMINATE_MSG Msg;
}MY_DRIVER_PROCESS_TERMINATE_MSG_FULL, *PMY_DRIVER_PROCESS_TERMINATE_MSG_FULL;


#pragma pack(pop)
#pragma endregion Messages


#endif