#ifndef _NOTIFICATIONS_H_
#define _NOTIFICATIONS_H_

#include <ntddk.h>
#include <minwindef.h>

NTSTATUS
Drv0InitializeNotifications(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
Drv0UninitializeNotifications(
    VOID
);

#endif