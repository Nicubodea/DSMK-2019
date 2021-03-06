#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <ntddk.h>
#include <minwindef.h>

#define TAG_THRPOOL_KTHR        'RHTK'
#define TAG_THRPOOL_KPOOL       'LOPK'
#define TAG_THRPOOL_KWORK       'ROWK'

typedef enum _KTHREAD_POOL_STATE
{
    kthreadPoolUnknown,
    kthreadPoolStarted,
    kthreadPoolStopped
} KTHREAD_POOL_STATE;

typedef enum _KTHREAD_POOL_SYNC_TYPE
{
    kthreadPoolSyncTypeSpinLock,
    kthreadPoolSyncTypeEresource,
    kthreadPoolSyncTypePushLock
} KTHREAD_POOL_SYNC_TYPE;

typedef struct _KTHREAD_POOL
{
    HANDLE *Threads;
    DWORD NumberOfThreads;

    KTHREAD_POOL_SYNC_TYPE TypeOfSynchronization;
    KTHREAD_POOL_STATE ThreadPoolState;

    LIST_ENTRY WorkQueue;

    KSPIN_LOCK QueueSpinLock;
    ERESOURCE QueueResource;
    EX_PUSH_LOCK QueuePushLock;

    KEVENT QueueSignalEvent;
    KEVENT QueueEmptyEvent;

} KTHREAD_POOL, *PKTHREAD_POOL;

typedef VOID* (*PFUNC_WorkFunction)(VOID* Param);
typedef VOID(*PFUNC_CompletionCallback)(VOID* Result);

typedef struct _KTHREAD_POOL_WORK_ITEM
{
    LIST_ENTRY Link;
    PFUNC_WorkFunction Function;
    PFUNC_CompletionCallback CompletionCallback;
    VOID* Param;

} KTHREAD_POOL_WORK_ITEM, *PKTHREAD_POOL_WORK_ITEM;

NTSTATUS
KThrpInitializeThreadPool(
    _In_ DWORD NumberOfThreads,
    _In_ KTHREAD_POOL_SYNC_TYPE SyncType,
    _Inout_ KTHREAD_POOL** ThreadPool
);

NTSTATUS
KThrpCreateAndEnqueueWorkItem(
    _In_ KTHREAD_POOL* ThreadPool,
    _In_ PFUNC_WorkFunction Function,
    _In_ PFUNC_CompletionCallback CompletionCallback,
    _In_ VOID* Param
);

NTSTATUS
KThrpWaitAndStopThreadPool(
    _In_ KTHREAD_POOL* ThreadPool
);

VOID
KThrpWaitForWorkItemsToFinish(
    _In_ KTHREAD_POOL* ThreadPool
);

#endif