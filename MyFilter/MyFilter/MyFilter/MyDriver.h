#ifndef _MY_DRIVER_HPP_INCLUDED_
#define _MY_DRIVER_HPP_INCLUDED_
//

//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#include "Communication.h"
#include "threadpool.h"

typedef struct _GLOBLA_DATA
{
    PFLT_FILTER FilterHandle;
    DRIVER_OBJECT *DrvObj;
    APP_COMMUNICATION Communication;
    ULONG Options;
    KTHREAD_POOL *ThreadPool;
}GLOBLA_DATA, *PGLOBLA_DATA;

extern GLOBLA_DATA gDrv;

#endif