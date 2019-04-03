#ifndef _PROT_PROC_H_
#define _PROT_PROC_H_

#include <ntddk.h>
#include <minwindef.h>
#include "sioctl.h"

NTSTATUS
Drv0ProtectProcess(
    _In_ IOCTL_PROT_UNPROT_PROC_STRUCT* ProtStruct
);


NTSTATUS
Drv0UnprotectProcess(
    _In_ IOCTL_PROT_UNPROT_PROC_STRUCT* ProtStruct
);

NTSTATUS
Drv0InitializeProcessProtection(
    VOID
);

NTSTATUS
Drv0UnitializeProcessProtection(
    VOID
);


#endif