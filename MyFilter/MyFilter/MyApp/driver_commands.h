#ifndef _COMMANDS_H_
#define _COMMANDS_H_
//
//   Author(s)    : Radu PORTASE(rportase@bitdefender.com)
//
#include "CommShared.h"

//
// CmdGetDriverVersion
//
NTSTATUS
CmdGetDriverVersion(
    _Out_ PULONG DriverVersion
    );

//
// CmdStartMonitoring
//

NTSTATUS
CmdStartMonitoring(
    DWORD MonitorMask
    );

//
// CmdStopMonitoring
//

NTSTATUS
CmdStopMonitoring(
    DWORD StopMask
    );

#endif//_COMMANDS_H_