
#define MY_IOCTL_TYPE 40000


#define MY_IOCTL_CODE_FIRST                 CTL_CODE( MY_IOCTL_TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define MY_IOCTL_CODE_SECOND                CTL_CODE( MY_IOCTL_TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define MY_IOCTL_CODE_TEST_KTHREAD_POOL     CTL_CODE( MY_IOCTL_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS  )


#define DRIVER_NAME "DSMKDriver0"