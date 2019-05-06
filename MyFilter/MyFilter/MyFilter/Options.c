#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "Trace.h"
#include "Options.tmh"
#include "Options.h"
#include "MyDriver.h"

VOID
MyFltUpdateOptions(
    ULONG NewOptions
)
{
    LogInfo("[INFO] Requested to change options from %x to %x", gDrv.Options, NewOptions);

    gDrv.Options = NewOptions;
}