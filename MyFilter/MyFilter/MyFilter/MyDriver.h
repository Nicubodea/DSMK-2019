#ifndef _MY_DRIVER_HPP_INCLUDED_
#define _MY_DRIVER_HPP_INCLUDED_
//

//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#include "Communication.h"

typedef struct _GLOBLA_DATA
{
    PFLT_FILTER FilterHandle;
    APP_COMMUNICATION Communication;
    ULONG MonitoringStarted;
}GLOBLA_DATA, *PGLOBLA_DATA;

extern GLOBLA_DATA gDrv;

#endif