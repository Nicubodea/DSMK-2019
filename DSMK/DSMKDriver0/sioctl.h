#ifndef _SIOCTL_H_
#define _SIOCTL_H_

#ifdef KERNEL
#include <ntddk.h>
#include <minwindef.h>
#endif

#define MY_IOCTL_TYPE 40000

#define MY_IOCTL_CODE_FIRST                 CTL_CODE( MY_IOCTL_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define MY_IOCTL_CODE_SECOND                CTL_CODE( MY_IOCTL_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define MY_IOCTL_CODE_TEST_KTHREAD_POOL     CTL_CODE( MY_IOCTL_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define MY_IOCTL_CODE_PROTECT_PROCESS       CTL_CODE( MY_IOCTL_TYPE, 0x903, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define MY_IOCTL_CODE_UNPROTECT_PROCESS     CTL_CODE( MY_IOCTL_TYPE, 0x904, METHOD_BUFFERED, FILE_ANY_ACCESS  )


#pragma pack(push)
#pragma pack(4)

#pragma warning(push)
#pragma warning(disable:4200)

typedef struct _IOCTL_GET_SET_STRUCT
{
    NTSTATUS Status;
    DWORD BufferSize;
    BYTE Buffer[0];
} IOCTL_GET_SET_STRUCT, *PIOCTL_GET_SET_STRUCT;

typedef struct _IOCTL_PROT_UNPROT_PROC_STRUCT
{
    DWORD Pid;
} IOCTL_PROT_UNPROT_PROC_STRUCT, *PIOCTL_PROT_UNPROT_PROC_STRUCT;

#pragma warning(pop)

#pragma pack(pop)


#define DRIVER_NAME "DSMKDriver0"

#endif