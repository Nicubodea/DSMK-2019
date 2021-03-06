#include <ntddk.h>          
#include <string.h>

#include "sioctl.h"

#include "trace.h"

#include "threadpool.h"

#include "protproc.h"

#include "notifications.h"

#include "./x64/Debug/drivermain.tmh"

#define NT_DEVICE_NAME      L"\\Device\\Drv0"
#define DOS_DEVICE_NAME     L"\\DosDevices\\Drv0File"

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH Drv0CreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH Drv0DeviceControl;

DRIVER_UNLOAD Drv0UnloadDriver;

__forceinline int __min(
    int a, int b
)
{
    if (a < b) return a;
    return b;
}

#define MIN(a, b) __min((a), (b))


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    UNICODE_STRING ntUnicodeString;    
    UNICODE_STRING ntWin32NameString;   
    PDEVICE_OBJECT deviceObject = NULL;
    BOOLEAN bShouldWppCleanup = FALSE;
    BOOLEAN bShouldDeleteDevice = FALSE;
    BOOLEAN bShouldDeleteSymlink = FALSE;
    BOOLEAN bShouldDeleteNotifications = FALSE;
    BOOLEAN bShouldUninitProcess = FALSE;

    UNREFERENCED_PARAMETER(RegistryPath);

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    bShouldWppCleanup = TRUE;

    Drv0LogInfo("DriverEntry called");
        
    RtlInitUnicodeString(&ntUnicodeString, NT_DEVICE_NAME);

    status = IoCreateDevice(
        DriverObject,            
        0,                       
        &ntUnicodeString,        
        FILE_DEVICE_UNKNOWN,     
        FILE_DEVICE_SECURE_OPEN, 
        FALSE,                   
        &deviceObject);          

    if (!NT_SUCCESS(status))
    {
        Drv0LogError("Couldn't create the device object\n");
        goto cleanup_and_exit;
    }

    bShouldDeleteDevice = TRUE;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = Drv0CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Drv0CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Drv0DeviceControl;
    DriverObject->DriverUnload = Drv0UnloadDriver;

    RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

    status = IoCreateSymbolicLink(
        &ntWin32NameString, &ntUnicodeString);

    if (!NT_SUCCESS(status))
    {
        Drv0LogError("Couldn't create symbolic link\n");
        goto cleanup_and_exit;
    }

    bShouldDeleteSymlink = TRUE;

    status = Drv0InitializeNotifications(DriverObject);
    if (!NT_SUCCESS(status))
    {
        Drv0LogError("[ERROR] Drv0InitializeNotifications failed: 0x%08x\n", status);
        goto cleanup_and_exit;
    }

    bShouldDeleteNotifications = TRUE;

    status = Drv0InitializeProcessProtection();
    if (!NT_SUCCESS(status))
    {
        Drv0LogError("[ERROR] Drv0InitializeProcessProtection failed: 0x%08x\n", status);
        goto cleanup_and_exit;
    }

    bShouldUninitProcess = TRUE;

cleanup_and_exit:
    if (!NT_SUCCESS(status))
    {
        if (bShouldUninitProcess)
        {
            Drv0UnitializeProcessProtection();
        }

        if (bShouldDeleteNotifications)
        {
            Drv0UninitializeNotifications();
        }

        if (bShouldDeleteSymlink)
        {
            IoDeleteSymbolicLink(&ntWin32NameString);
        }

        if (bShouldDeleteDevice)
        {
            IoDeleteDevice(deviceObject);
        }

        if (bShouldWppCleanup)
        {
            WPP_CLEANUP(DriverObject);
        }
    }

    return status;
}


NTSTATUS
Drv0CreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID
Drv0UnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING uniWin32NameString;

    PAGED_CODE();

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString(&uniWin32NameString, DOS_DEVICE_NAME);


    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    IoDeleteSymbolicLink(&uniWin32NameString);

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

    Drv0UninitializeNotifications();

    Drv0UnitializeProcessProtection();

    WPP_CLEANUP(DriverObject);
}


KMUTEX gMutex;


static VOID
_Drv0DefaultCompletionCallback(
    VOID* Param
)
{
    UNREFERENCED_PARAMETER(Param);
}


static VOID*
_Drv0DefaultWorkCallback(
    VOID* Param
)
{
    int* pInt = (int*)Param;

    KeWaitForSingleObject(&gMutex, Executive, KernelMode, FALSE, NULL);

    *pInt += 1;

    KeReleaseMutex(&gMutex, FALSE);

    return NULL;
}



NTSTATUS
Drv0TestThreadPool(
    VOID
)
{
    KTHREAD_POOL *threadPool;
    NTSTATUS status;
    int a = 0;

    KeInitializeMutex(&gMutex, 0);

    status = KThrpInitializeThreadPool(10, kthreadPoolSyncTypeSpinLock, &threadPool);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (int i = 0; i < 100; i++)
    {
        status = KThrpCreateAndEnqueueWorkItem(threadPool, _Drv0DefaultWorkCallback, _Drv0DefaultCompletionCallback, &a);
        if (!NT_SUCCESS(status))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    KThrpWaitForWorkItemsToFinish(threadPool);

    status = KThrpWaitAndStopThreadPool(threadPool);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (a != 100)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = KThrpInitializeThreadPool(10, kthreadPoolSyncTypeEresource, &threadPool);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (int i = 0; i < 100; i++)
    {
        status = KThrpCreateAndEnqueueWorkItem(threadPool, _Drv0DefaultWorkCallback, _Drv0DefaultCompletionCallback, &a);
        if (!NT_SUCCESS(status))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    KThrpWaitForWorkItemsToFinish(threadPool);

    status = KThrpWaitAndStopThreadPool(threadPool);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (a != 200)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = KThrpInitializeThreadPool(10, kthreadPoolSyncTypePushLock, &threadPool);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (int i = 0; i < 100; i++)
    {
        status = KThrpCreateAndEnqueueWorkItem(threadPool, _Drv0DefaultWorkCallback, _Drv0DefaultCompletionCallback, &a);
        if (!NT_SUCCESS(status))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    KThrpWaitForWorkItemsToFinish(threadPool);

    status = KThrpWaitAndStopThreadPool(threadPool);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (a != 300)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
Drv0DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG inBufLength;
    ULONG outBufLength;
    char *inbuf, *outbuf;
    DWORD dataLen;
    IOCTL_GET_SET_STRUCT* ioStruct;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Drv0LogInfo("Drv0DeviceControl called with %x %d %d\n", irpSp->Parameters.DeviceIoControl.IoControlCode, inBufLength, outBufLength);

    if (!inBufLength || !outBufLength)
    {
        status = STATUS_INVALID_PARAMETER;
        Drv0LogError("0 sized buffers given!");
        Irp->IoStatus.Status = status;

        goto end;
    }

    inbuf = Irp->AssociatedIrp.SystemBuffer;
    outbuf = Irp->AssociatedIrp.SystemBuffer;

    ioStruct = (IOCTL_GET_SET_STRUCT*)inbuf;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case MY_IOCTL_CODE_FIRST:
        {
            Drv0LogInfo("First IOCTL was called!");
            break;
        }
        case MY_IOCTL_CODE_SECOND:
        {
            Drv0LogInfo("Second IOCTL was called!");
            break;
        }
        case MY_IOCTL_CODE_TEST_KTHREAD_POOL:
        {
            status = Drv0TestThreadPool();
            if (!NT_SUCCESS(status))
            {
                Drv0LogError("Thread pool test failed!");
                goto end;
            }
            Drv0LogInfo("Thread pool test passed!");
            break;
        }
        case MY_IOCTL_CODE_PROTECT_PROCESS:
        {
            status = Drv0ProtectProcess((IOCTL_PROT_UNPROT_PROC_STRUCT*)ioStruct->Buffer);
            if (!NT_SUCCESS(status))
            {
                Drv0LogError("[ERROR] Drv0ProtectProcess failed: 0x%08x\n", status);
            }
            break;
        }

        case MY_IOCTL_CODE_UNPROTECT_PROCESS:
        {
            status = Drv0UnprotectProcess((IOCTL_PROT_UNPROT_PROC_STRUCT*)ioStruct->Buffer);
            if (!NT_SUCCESS(status))
            {
                Drv0LogError("[ERROR] Drv0UnprotectProcess failed: 0x%08x\n", status);
            }
            break;
        }
        default:
        {
            Drv0LogError("[ERROR] Unrecognized ioctl: %x\n", irpSp->Parameters.DeviceIoControl.IoControlCode);
            status = STATUS_NOT_FOUND;
            break;
        }
    }

    ioStruct->Status = status;

    dataLen = sizeof(IOCTL_GET_SET_STRUCT) + ioStruct->BufferSize;
    
    if (dataLen > outBufLength)
    {
        ioStruct->Status = STATUS_BUFFER_TOO_SMALL;
    }

    //RtlCopyBytes(outbuf, ioStruct, MIN(dataLen, outBufLength));

    Irp->IoStatus.Information = MIN(dataLen, outBufLength);
    Irp->IoStatus.Status = ioStruct->Status;
end:
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}
