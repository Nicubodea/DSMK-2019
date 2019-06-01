
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "Trace.h"
#include "MyFilter.tmh"
#include "MyDriver.h"
#include "Communication.h"
#include "Options.h"
#include "CommShared.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")
/*************************************************************************
    Globals
*************************************************************************/
GLOBLA_DATA gDrv;


/*************************************************************************
    Prototypes
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
MyFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
MyFilterInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
MyFilterInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
MyFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
MyFilterInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
MyFilterPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
MyFilterPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
MyFilterPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MyFilterUnload)
#pragma alloc_text(PAGE, MyFilterInstanceQueryTeardown)
#pragma alloc_text(PAGE, MyFilterInstanceSetup)
#pragma alloc_text(PAGE, MyFilterInstanceTeardownStart)
#pragma alloc_text(PAGE, MyFilterInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_CLOSE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_READ,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_WRITE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    MyFilterUnload,                           //  MiniFilterUnload

    MyFilterInstanceSetup,                    //  InstanceSetup
    MyFilterInstanceQueryTeardown,            //  InstanceQueryTeardown
    MyFilterInstanceTeardownStart,            //  InstanceTeardownStart
    MyFilterInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
MyFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceSetup: Entered\n");

    return STATUS_SUCCESS;
}


NTSTATUS
MyFilterInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceQueryTeardown: Entered\n");

    return STATUS_SUCCESS;
}


VOID
MyFilterInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceTeardownStart: Entered\n");
}


VOID
MyFilterInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterInstanceTeardownComplete: Entered\n");
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    gDrv.DrvObj = DriverObject;

    LogInfo("MyFilter!DriverEntry: Entered\n");

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = KThrpInitializeThreadPool(10, kthreadPoolSyncTypeSpinLock, &gDrv.ThreadPool);
    if (!NT_SUCCESS(status))
    {
        LogError("KThrpInitializeThreadPool: 0x%08x\n", status);
        return status;
    }

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gDrv.FilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) 
    {
        //
        //  Prepare communication layer
        //
        status = CommInitializeFilterCommunicationPort();
        if (!NT_SUCCESS(status)) {

            FltUnregisterFilter(gDrv.FilterHandle);
            return status;
        }

        //
        //  Start filtering i/o
        //
        status = FltStartFiltering( gDrv.FilterHandle );
        if (!NT_SUCCESS( status ))
        {
            CommUninitializeFilterCommunicationPort();
            FltUnregisterFilter( gDrv.FilterHandle );
        }
    }

    return status;
}

NTSTATUS
MyFilterUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    NTSTATUS status;

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterUnload: Entered\n");
    MyFltUpdateOptions(0);
    status = KThrpWaitAndStopThreadPool(gDrv.ThreadPool);
    if (!NT_SUCCESS(status))
    {
        LogCritical("KThrpWaitAndStopThreadPool: 0x%08x\n", status);
    }

    CommUninitializeFilterCommunicationPort();
    FltUnregisterFilter( gDrv.FilterHandle );

    WPP_CLEANUP(gDrv.DrvObj);

    return STATUS_SUCCESS;
}


VOID
MyFilterSendFileMessage(
    UNICODE_STRING* FileName,
    MY_DRIVER_MSG_FILE_TYPE Type
)
{
    MY_DRIVER_MSG_FILE_NOTIFICATION *pMsg = NULL;
    LARGE_INTEGER time;

    if (!(gDrv.Options & OPT_FLAG_MONITOR_FILE))
    {
        return;
    }

    ULONG msgSize = sizeof(MY_DRIVER_MSG_FILE_NOTIFICATION);
    if (FileName)
    {
        msgSize += FileName->Length;
    }

    pMsg = ExAllocatePoolWithTag(PagedPool, msgSize, 'GSM+');
    if (!pMsg)
    {
        return;
    }

    RtlZeroMemory(pMsg, sizeof(*pMsg));

    pMsg->Header.MessageCode = msgFileOp;

    KeQuerySystemTimePrecise(&time);
    
    pMsg->Header.TimeStamp = time.QuadPart;

    pMsg->Header.Result = STATUS_SUCCESS;

    pMsg->Type = Type;

    if (FileName)
    {
        pMsg->NameLen = FileName->Length;
        RtlCopyMemory(&pMsg->Data[0], FileName->Buffer, FileName->Length);
    }
    else
    {
        pMsg->NameLen = 0;
    }

    NTSTATUS status = CommSendMessageOnThreadPool(pMsg, msgSize, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        LogError("CommSendMessage failed with status = 0x%X", status);
    }

    //ExFreePoolWithTag(pMsg, 'GSM+');
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
MyFilterPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

    PFLT_FILE_NAME_INFORMATION fltInfo;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    if (!(gDrv.Options & OPT_FLAG_MONITOR_FILE))
    {
        return FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &fltInfo);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] FltGetFileNameInformation: 0x%08x", status);
        goto cleanup_and_exit;
    }

    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
    {
        MyFilterSendFileMessage(&fltInfo->Name, fileCreate);
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_CLOSE)
    {
        MyFilterSendFileMessage(&fltInfo->Name, fileClose);
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_CLEANUP)
    {
        MyFilterSendFileMessage(&fltInfo->Name, fileCleanup);
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_READ)
    {
        MyFilterSendFileMessage(&fltInfo->Name, fileRead);
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_WRITE)
    {
        MyFilterSendFileMessage(&fltInfo->Name, fileWrite);
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
    {
        MyFilterSendFileMessage(&fltInfo->Name, fileSetAttributes);
    }

cleanup_and_exit:

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
MyFilterPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    //LogInfo("MyFilter!MyFilterPostOperation: Entered\n");
    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
MyFilterPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    LogInfo("MyFilter!MyFilterPreOperationNoPostOperation: Entered\n");

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}