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

#define ACTIVATED(Option, Options) (!!((Options) & (Option)))
#define NOT_ACTIVATED(Option, Options) (!((Options) & (Option)))

#define NOT_ACTIVATED_BUT_WILL_ACTIVATE(Option, NewOptions) (NOT_ACTIVATED((Option), gDrv.Options) && ACTIVATED((Option), (NewOptions)))


VOID
MyFltUpdateOptions(
    ULONG NewOptions
)
{
    NTSTATUS status;

    LogInfo("[INFO] Requested to change options from %x to %x", gDrv.Options, NewOptions);

    if (NOT_ACTIVATED_BUT_WILL_ACTIVATE(OPT_FLAG_MONITOR_CREATE_PROCESS, NewOptions) ||
        NOT_ACTIVATED_BUT_WILL_ACTIVATE(OPT_FLAG_MONITOR_TERMINATE_PROCESS, NewOptions))
    {
        status = ProcFltInitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ProcFltInitialize: %08x", status);
        }

        status = ThrFltInitialize(gDrv.DrvObj);
        if (!NT_SUCCESS(status))
        {
            LogError("ThrFltInitialize: 0x%08x", status);
        }

        status = ModFltInitialize(gDrv.DrvObj);
        if (!NT_SUCCESS(status))
        {
            LogError("ModFltInitialize: 0x%08x", status);
        }

        status = RegFltInitialize(gDrv.DrvObj);
        if (!NT_SUCCESS(status))
        {
            LogError("RegFltInitialize: 0x%08x", status);
        }
    }

    if (ACTIVATED(OPT_FLAG_MONITOR_CREATE_PROCESS | OPT_FLAG_MONITOR_TERMINATE_PROCESS, gDrv.Options) &&
        NOT_ACTIVATED(OPT_FLAG_MONITOR_CREATE_PROCESS, NewOptions) &&
        NOT_ACTIVATED(OPT_FLAG_MONITOR_TERMINATE_PROCESS, NewOptions))
    {
        status = ProcFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ProcFltUninitialize: %08x", status);
        }

        status = ThrFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ThrFltUninitialize: 0x%08x", status);
        }

        status = ModFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("ModFltUninitialize: 0x%08x", status);
        }

        status = RegFltUninitialize();
        if (!NT_SUCCESS(status))
        {
            LogError("RegFltUninitialize: %08x", status);
        }
    }

    

    gDrv.Options = NewOptions;
}