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
    );

//
// CmdStopMonitoring
//

NTSTATUS
CmdStopMonitoring(
    );

#endif//_COMMANDS_H_