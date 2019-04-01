#include <ntddk.h>          
#include <string.h>

#include "sioctl.h"

#include "trace.h"
#include "./x64/Debug/drivermain.tmh"

#define NT_DEVICE_NAME      L"\\Device\\Drv2"

DRIVER_INITIALIZE DriverEntry;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH Drv2CreateClose;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH Drv2DeviceControl;

DRIVER_UNLOAD Drv2UnloadDriver;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS        status;
    UNICODE_STRING  ntUnicodeString;
    PDEVICE_OBJECT  deviceObject = NULL;

    UNREFERENCED_PARAMETER(RegistryPath);

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    Drv2LogInfo("DriverEntry called");

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
        Drv2LogError("Couldn't create the device object\n");
        goto cleanup_and_exit;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = Drv2CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Drv2CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Drv2DeviceControl;
    DriverObject->DriverUnload = Drv2UnloadDriver;

   
cleanup_and_exit:
    if (!NT_SUCCESS(status))
    {
        WPP_CLEANUP(DriverObject);
    }

    return status;
}


NTSTATUS
Drv2CreateClose(
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
Drv2UnloadDriver(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;

    PAGED_CODE();
    

    if (deviceObject != NULL)
    {
        IoDeleteDevice(deviceObject);
    }

    WPP_CLEANUP(DriverObject);
}



NTSTATUS
Drv2DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               inBufLength;
    ULONG               outBufLength;

    UNREFERENCED_PARAMETER(DeviceObject);



    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    Drv2LogInfo("Drv2DeviceControl called with %d", irpSp->Parameters.DeviceIoControl.IoControlCode);

    if (!inBufLength || !outBufLength)
    {
        status = STATUS_INVALID_PARAMETER;
        Drv2LogError("0 sized buffers given!");
        goto end;
    }



    switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
    {
    case MY_IOCTL_CODE_FIRST:
    {
        Drv2LogInfo("First IOCTL was called!");
    }
    case MY_IOCTL_CODE_SECOND:
    {
        Drv2LogInfo("Second IOCTL was called!");
    }
    }

end:

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}
