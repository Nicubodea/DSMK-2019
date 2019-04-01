#include <ntddk.h>          
#include <string.h>

#include "sioctl.h"

#include "trace.h"
#include "./x64/Debug/drivermain.tmh"

#define NT_DEVICE_NAME      L"\\Device\\Drv1"
#define DOS_DEVICE_NAME     L"\\DosDevices\\Drv1File"

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH Drv1CreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH Drv1DeviceControl;

DRIVER_UNLOAD Drv1UnloadDriver;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS        status;
    UNICODE_STRING  ntUnicodeString;
    UNICODE_STRING  ntWin32NameString;
    PDEVICE_OBJECT  deviceObject = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    Drv1LogInfo("DriverEntry called");

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
        Drv1LogError("Couldn't create the device object\n");
        goto cleanup_and_exit;
    }

    deviceObject->StackSize++;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = Drv1CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Drv1CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Drv1DeviceControl;
    DriverObject->DriverUnload = Drv1UnloadDriver;

    RtlInitUnicodeString(&ntWin32NameString, DOS_DEVICE_NAME);

    status = IoCreateSymbolicLink(
        &ntWin32NameString, &ntUnicodeString);

    if (!NT_SUCCESS(status))
    {
        Drv1LogError("Couldn't create symbolic link\n");
        IoDeleteDevice(deviceObject);
    }

cleanup_and_exit:
    if (!NT_SUCCESS(status))
    {
        WPP_CLEANUP(DriverObject);
    }

    return status;
}


NTSTATUS
Drv1CreateClose(
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
Drv1UnloadDriver(
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

    WPP_CLEANUP(DriverObject);
}


NTSTATUS
Drv1CompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(DeviceObject);

    Drv1LogInfo("Completion status: %d", Irp->IoStatus.Status);

    return STATUS_SUCCESS;
}



NTSTATUS
Drv1DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               inBufLength;
    ULONG               outBufLength;
    UNICODE_STRING driver2;
    PFILE_OBJECT fobj;
    PDEVICE_OBJECT dobj;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Drv1LogInfo("Drv1DeviceControl called with %d", irpSp->Parameters.DeviceIoControl.IoControlCode);

    if (!inBufLength || !outBufLength)
    {
        status = STATUS_INVALID_PARAMETER;
        Drv1LogError("0 sized buffers given!");
        goto end;
    }

    RtlInitUnicodeString(&driver2, L"\\Device\\Drv2");

    status = IoGetDeviceObjectPointer(&driver2, FILE_ALL_ACCESS, &fobj, &dobj);
    if (!NT_SUCCESS(status))
    {
        Drv1LogError("IoGetDeviceObjectPointer failed: 0x%08x", status);
        goto end;
    }

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, Drv1CompletionRoutine, NULL, TRUE, TRUE, TRUE);

    status = IoCallDriver(dobj, Irp);
    

    ObDereferenceObject(dobj);

end:
    return status;
}
