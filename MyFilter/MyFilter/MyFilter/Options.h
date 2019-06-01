#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <fltKernel.h>

#define OPT_FLAG_MONITOR_PROCESS                        0x00000001
#define OPT_FLAG_MONITOR_THREAD                         0x00000002
#define OPT_FLAG_MONITOR_MODULE_LOAD                    0x00000004
#define OPT_FLAG_MONITOR_REGISTRY                       0x00000008                
#define OPT_FLAG_MONITOR_FILE                           0x00000010

#define ACTIVATED(Option, NewOptions) (!!((NewOptions) & (Option)) && !(gDrv.Options & (Option)))
#define NOT_ACTIVATED(Option, NewOptions) (!((NewOptions) & (Option)) && !!(gDrv.Options & (Option)))

VOID
MyFltUpdateOptions(
    ULONG OldOptions
);

#endif