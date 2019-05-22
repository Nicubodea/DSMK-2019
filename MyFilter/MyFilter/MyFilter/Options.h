#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <fltKernel.h>

#define OPT_FLAG_MONITOR_CREATE_PROCESS                 0x00000001
#define OPT_FLAG_MONITOR_TERMINATE_PROCESS              0x00000002
#define OPT_FLAG_MONITOR_THREAD_CREATE                  0x00000004
#define OPT_FLAG_MONITOR_THREAD_TERMINATE               0x00000008
#define OPT_FLAG_MONITOR_MODULE_LOAD                    0x00000010
#define OPT_FLAG_MONITOR_REGISTRY                       0x00000020                
#define OPT_FLAG_MONITOR_FILE                           0x00000040

VOID
MyFltUpdateOptions(
    ULONG OldOptions
);

#endif