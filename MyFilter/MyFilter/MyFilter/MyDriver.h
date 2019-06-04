#ifndef _MY_DRIVER_HPP_INCLUDED_
#define _MY_DRIVER_HPP_INCLUDED_
//

//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#include "Communication.h"
#include "threadpool.h"
#include <fltKernel.h>
#include <ntddk.h>
#include "Fwpmk.h"

#pragma warning(push)
#pragma warning(disable:4201) // unnamed struct/union

#include "Fwpsk.h"

#pragma warning(pop)



typedef struct _GLOBLA_DATA
{
    PFLT_FILTER FilterHandle;
    DRIVER_OBJECT *DrvObj;
    APP_COMMUNICATION Communication;
    ULONG Options;
    KTHREAD_POOL *ThreadPool;
    DEVICE_OBJECT *DeviceObject;
    HANDLE EngineHandle;

    UINT64 FilterId;
    UINT32 SCalloutId;
    UINT32 MCalloutId;

}GLOBLA_DATA, *PGLOBLA_DATA;

extern GLOBLA_DATA gDrv;

#endif