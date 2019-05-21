#ifndef _REGISTER_FILTER_H_
#define _REGISTER_FILTER_H_

#include "MyDriver.h"

NTSTATUS
RegFltInitialize(_In_ PDRIVER_OBJECT DriverObject);

NTSTATUS
RegFltUninitialize();

#endif