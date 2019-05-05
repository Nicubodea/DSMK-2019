
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "Trace.h"
#include "MyFilter.tmh"
#include "MyDriver.h"
#include "Communication.h"
#include "ProcessFilter.h"

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

#if 0 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
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

    { IRP_MJ_QUERY_INFORMATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SET_EA,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      MyFilterPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_PNP,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      MyFilterPreOperation,
      MyFilterPostOperation },

#endif // TODO

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

    LogInfo("MyFilter!DriverEntry: Entered\n");

    //
    //  Register with FltMgr to tell it our callback routines
    //

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

        status = ProcFltInitialize();
        if (!NT_SUCCESS(status))
        {
            CommUninitializeFilterCommunicationPort();
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
            ProcFltUninitialize();
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

    PAGED_CODE();

    LogInfo("MyFilter!MyFilterUnload: Entered\n");
    CommUninitializeFilterCommunicationPort();
    ProcFltUninitialize();
    FltUnregisterFilter( gDrv.FilterHandle );

    return STATUS_SUCCESS;
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
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    LogInfo("MyFilter!MyFilterPreOperation: Entered\n");
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

    LogInfo("MyFilter!MyFilterPostOperation: Entered\n");
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