#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "Trace.h"
#include "Options.tmh"
#include "Options.h"
#include "MyDriver.h"

#include "ProcessFilter.h"
#include "ThreadFIlter.h"
#include "ModuleFilter.h"
#include "RegisterFilter.h"

VOID
MyFltUpdateOptions(
    ULONG NewOptions
)
{
    NTSTATUS status;

    LogInfo("[INFO] Requested to change options from %x to %x", gDrv.Options, NewOptions);

    if (ACTIVATED(OPT_FLAG_MONITOR_PROCESS, NewOptions))
    {
        status = ProcFltInitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ProcFltInitialize: %08x", status);
        }
    }

    if (ACTIVATED(OPT_FLAG_MONITOR_THREAD, NewOptions))
    {
        status = ThrFltInitialize(gDrv.DrvObj);
        if (!NT_SUCCESS(status))
        {
            LogError("ThrFltInitialize: 0x%08x", status);
        }
    }

    if (ACTIVATED(OPT_FLAG_MONITOR_MODULE_LOAD, NewOptions))
    {
        status = ModFltInitialize(gDrv.DrvObj);
        if (!NT_SUCCESS(status))
        {
            LogError("ModFltInitialize: 0x%08x", status);
        }
    }

    if (ACTIVATED(OPT_FLAG_MONITOR_REGISTRY, NewOptions))
    {
        status = RegFltInitialize(gDrv.DrvObj);
        if (!NT_SUCCESS(status))
        {
            LogError("RegFltInitialize: 0x%08x", status);
        }
    }

    
    if (NOT_ACTIVATED(OPT_FLAG_MONITOR_PROCESS, NewOptions))
    {
        status = ProcFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ProcFltUninitialize: %08x", status);
        }
    }

    if (NOT_ACTIVATED(OPT_FLAG_MONITOR_THREAD, NewOptions))
    {
        status = ThrFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ThrFltUninitialize: 0x%08x", status);
        }
    }

    if (NOT_ACTIVATED(OPT_FLAG_MONITOR_MODULE_LOAD, NewOptions))
    {
        status = ModFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ModFltUninitialize: 0x%08x", status);
        }
    }

    if (NOT_ACTIVATED(OPT_FLAG_MONITOR_REGISTRY, NewOptions))
    {
        status = RegFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("RegFltUninitialize: %08x", status);
        }
    }

    gDrv.Options = NewOptions;
}