#include "threadpool.h"
#include "Trace.h"

#include "threadpool.tmh"


#define ACQUIRE_LOCK(ThreadPool, OldIrql) \
 switch (ThreadPool->TypeOfSynchronization)\
 {\
        case kthreadPoolSyncTypeSpinLock:\
        {\
            KeAcquireSpinLock(&ThreadPool->QueueSpinLock, &OldIrql);\
            break;\
        }\
        case kthreadPoolSyncTypeEresource:\
        {\
            ExEnterCriticalRegionAndAcquireResourceExclusive(&ThreadPool->QueueResource);\
            break;\
        }\
        case kthreadPoolSyncTypePushLock:\
        {\
            ExAcquirePushLockExclusive(&ThreadPool->QueuePushLock);\
            break;\
        }\
 }\


#define RELEASE_LOCK(ThreadPool, OldIrql) \
switch (ThreadPool->TypeOfSynchronization)\
{\
        case kthreadPoolSyncTypeSpinLock:\
        {\
            KeReleaseSpinLock(&ThreadPool->QueueSpinLock, OldIrql);\
            break;\
        }\
        case kthreadPoolSyncTypeEresource:\
        {\
            ExReleaseResourceAndLeaveCriticalRegion(&ThreadPool->QueueResource);\
            break;\
        }\
        case kthreadPoolSyncTypePushLock:\
        {\
            ExReleasePushLockExclusive(&ThreadPool->QueuePushLock);\
            break;\
        }\
}\


//
// _KThrpDefaultThreadExecRoutine
//
static VOID
_KThrpDefaultThreadExecRoutine(
    _In_ KTHREAD_POOL* ThreadPool
)
{
    UNREFERENCED_PARAMETER(ThreadPool);
    NTSTATUS status;
    KIRQL oldIrql = KeGetCurrentIrql();

    while (TRUE)
    {
        if (ThreadPool->ThreadPoolState == kthreadPoolStopped)
        {
            goto _exit_from_func;
        }

        status = KeWaitForSingleObject(&ThreadPool->QueueSignalEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);
        if (status != STATUS_SUCCESS)
        {
            LogWarning("[WARNING] KeWaitForSingleObject returned: 0x%08x\n", status);
        }

        ACQUIRE_LOCK(ThreadPool, oldIrql);

        if (IsListEmpty(&ThreadPool->WorkQueue))
        {
            KeResetEvent(&ThreadPool->QueueSignalEvent);
            KeSetEvent(&ThreadPool->QueueEmptyEvent, 0, FALSE);
            RELEASE_LOCK(ThreadPool, oldIrql);

            continue;
        }

        KTHREAD_POOL_WORK_ITEM* workItem = CONTAINING_RECORD(ThreadPool->WorkQueue.Flink, KTHREAD_POOL_WORK_ITEM, Link);

        RemoveEntryList(&workItem->Link);

        RELEASE_LOCK(ThreadPool, oldIrql);

        VOID* result = workItem->Function(workItem->Param);
        
        if (NULL != workItem->CompletionCallback)
        {
            workItem->CompletionCallback(result);
        }
        ExFreePoolWithTag(workItem, TAG_THRPOOL_KWORK);
    }

_exit_from_func:
    return;
}


//
// KThrpInitializeThreadPool
//
NTSTATUS
KThrpInitializeThreadPool(
    _In_ DWORD NumberOfThreads,
    _In_ KTHREAD_POOL_SYNC_TYPE SyncType,
    _Inout_ KTHREAD_POOL** ThreadPool
)
{
    NTSTATUS status = STATUS_SUCCESS;
    KTHREAD_POOL *pThreadPool = NULL;

    if (0 == NumberOfThreads)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (NULL == ThreadPool)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    pThreadPool = ExAllocatePoolWithTag(NonPagedPool, sizeof(KTHREAD_POOL), TAG_THRPOOL_KPOOL);
    if (NULL == pThreadPool)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    switch (SyncType)
    {
    case kthreadPoolSyncTypeSpinLock:
    {
        KeInitializeSpinLock(&pThreadPool->QueueSpinLock);
        break;
    }
    case kthreadPoolSyncTypeEresource:
    {
        status = ExInitializeResourceLite(&pThreadPool->QueueResource);
        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] ExInitializeResourceLite: 0x%08x", status);
            goto cleanup_and_exit;
        }
        break;
    }
    case kthreadPoolSyncTypePushLock:
    {
        ExInitializePushLock(&pThreadPool->QueuePushLock);
        break;
    }
    default:
    {
        LogError("[ERROR] Unknown sync type: %d", SyncType);
        status = STATUS_INVALID_PARAMETER_2;
        goto cleanup_and_exit;
    }
    }

    pThreadPool->NumberOfThreads = NumberOfThreads;
    pThreadPool->TypeOfSynchronization = SyncType;

    KeInitializeEvent(&pThreadPool->QueueSignalEvent, NotificationEvent, FALSE);

    KeInitializeEvent(&pThreadPool->QueueEmptyEvent, NotificationEvent, TRUE);

    pThreadPool->Threads = ExAllocatePoolWithTag(NonPagedPool, NumberOfThreads * sizeof(HANDLE), TAG_THRPOOL_KTHR);
    if (NULL == pThreadPool->Threads)
    {
        LogError("[ERROR] ExAllocatePoolWithTag returned NULL");
        goto cleanup_and_exit;
    }

    InitializeListHead(&pThreadPool->WorkQueue);

    pThreadPool->ThreadPoolState = kthreadPoolStarted;

    for (DWORD i = 0; i < NumberOfThreads; i++)
    {
        status = PsCreateSystemThread(&pThreadPool->Threads[i],
            0,
            NULL,
            0,
            0,
            _KThrpDefaultThreadExecRoutine,
            pThreadPool);

        if (!NT_SUCCESS(status))
        {
            LogError("[ERROR] PsCreateSystemThread failed: 0x%08x\n", status);
            goto cleanup_and_exit;
        }
    }

cleanup_and_exit:
    if (!NT_SUCCESS(status))
    {
        pThreadPool->ThreadPoolState = kthreadPoolStopped;

        if (NULL != pThreadPool->Threads)
        {
            KeSetEvent(&pThreadPool->QueueSignalEvent, 0, TRUE);

            for (DWORD i = 0; i < NumberOfThreads; i++)
            {
                if (NULL != pThreadPool->Threads[i])
                {
                    VOID* object;
                    status = ObReferenceObjectByHandle(pThreadPool->Threads[i],
                        SYNCHRONIZE | EVENT_MODIFY_STATE,
                        *PsThreadType,
                        KernelMode,
                        &object,
                        NULL);
                    if (!NT_SUCCESS(status))
                    {
                        LogError("[ERROR] ObReferenceObjectByHandle failed: 0x%08x\n", status);
                        continue;
                    }

                    status = KeWaitForSingleObject(object, Executive, KernelMode, FALSE, NULL);
                    if (status != STATUS_SUCCESS)
                    {
                        LogWarning("[WARNING] KeWaitForSingleObject returned: 0x%08x\n", status);
                        status = STATUS_SUCCESS;
                    }

                    ZwClose(pThreadPool->Threads[i]);
                }
            }

            ExFreePoolWithTag(pThreadPool->Threads, TAG_THRPOOL_KTHR);
        }

        ExFreePoolWithTag(pThreadPool, TAG_THRPOOL_KPOOL);

        pThreadPool = NULL;
    }

    *ThreadPool = pThreadPool;

    return status;
}


//
// _KThrpEnqueueWorkInThreadPool
//
static NTSTATUS
_KThrpEnqueueWorkInThreadPool(
    _In_ KTHREAD_POOL* ThreadPool,
    _In_ KTHREAD_POOL_WORK_ITEM* WorkItem
)
{
    KIRQL oldIrql = KeGetCurrentIrql();

    if (NULL == ThreadPool)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (ThreadPool->ThreadPoolState != kthreadPoolStarted)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (NULL == WorkItem)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    ACQUIRE_LOCK(ThreadPool, oldIrql);

    InsertTailList(&ThreadPool->WorkQueue, &WorkItem->Link);

    RELEASE_LOCK(ThreadPool, oldIrql);

    KeSetEvent(&ThreadPool->QueueSignalEvent, 0, FALSE);

    KeResetEvent(&ThreadPool->QueueEmptyEvent);

    return STATUS_SUCCESS;
}


//
// KThrpCreateAndEnqueueWorkItem
//
NTSTATUS
KThrpCreateAndEnqueueWorkItem(
    _In_ KTHREAD_POOL* ThreadPool,
    _In_ PFUNC_WorkFunction Function,
    _In_ PFUNC_CompletionCallback CompletionCallback,
    _In_ VOID* Param
)
{
    KTHREAD_POOL_WORK_ITEM* workItem = NULL;
    NTSTATUS status;

    workItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(KTHREAD_POOL_WORK_ITEM), TAG_THRPOOL_KWORK);
    if (NULL == workItem)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    workItem->Function = Function;
    workItem->CompletionCallback = CompletionCallback;
    workItem->Param = Param;

    status = _KThrpEnqueueWorkInThreadPool(ThreadPool, workItem);
    if (!NT_SUCCESS(status))
    {
        LogError("[ERROR] _KThrpEnqueueWorkInThreadPool failed: 0x%08x", status);
        ExFreePoolWithTag(workItem, TAG_THRPOOL_KWORK);
    }

    return status;
}


//
// KThrpWaitAndStopThreadPool
//
NTSTATUS
KThrpWaitAndStopThreadPool(
    _In_ KTHREAD_POOL* ThreadPool
)
{
    NTSTATUS status;

    if (NULL == ThreadPool)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (ThreadPool->ThreadPoolState != kthreadPoolStarted)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    ThreadPool->ThreadPoolState = kthreadPoolStopped;

    if (NULL != ThreadPool->Threads)
    {
        KeSetEvent(&ThreadPool->QueueSignalEvent, 0, TRUE);

        for (DWORD i = 0; i < ThreadPool->NumberOfThreads; i++)
        {
            if (NULL != ThreadPool->Threads[i])
            {
                VOID* object;
                status = ObReferenceObjectByHandle(ThreadPool->Threads[i],
                    SYNCHRONIZE | EVENT_MODIFY_STATE,
                    *PsThreadType,
                    KernelMode,
                    &object,
                    NULL);
                if (!NT_SUCCESS(status))
                {
                    LogError("[ERROR] ObReferenceObjectByHandle failed: 0x%08x\n", status);
                    continue;
                }

                status = KeWaitForSingleObject(object, Executive, KernelMode, FALSE, NULL);
                if (status != STATUS_SUCCESS)
                {
                    LogWarning("[WARNING] KeWaitForSingleObject returned: 0x%08x\n", status);
                    status = STATUS_SUCCESS;
                }

                ZwClose(ThreadPool->Threads[i]);
            }
        }

        ExFreePoolWithTag(ThreadPool->Threads, TAG_THRPOOL_KTHR);
    }

    ExFreePoolWithTag(ThreadPool, TAG_THRPOOL_KPOOL);

    return STATUS_SUCCESS;
}


//
// KThrpWaitForWorkItemsToFinish
//
VOID
KThrpWaitForWorkItemsToFinish(
    _In_ KTHREAD_POOL* ThreadPool
)
{
    while (TRUE)
    {
        if (IsListEmpty(&ThreadPool->WorkQueue))
        {
            break;
        }

        KeWaitForSingleObject(&ThreadPool->QueueEmptyEvent, Executive, KernelMode, FALSE, NULL);
    }
}