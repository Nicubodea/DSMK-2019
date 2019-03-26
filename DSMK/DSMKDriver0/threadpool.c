#include "threadpool.h"
#include "trace.h"

#include "./x64/Debug/drivermain.tmh"


//
// _KThrpDefaultThreadExecRoutine
//
static VOID
_KThrpDefaultThreadExecRoutine(
    _In_ KTHREAD_POOL* ThreadPool
    )
{


}


//
// KThrpInitializeThreadPool
//
NTSTATUS
KThrpInitializeThreadPool(
    _In_ DWORD NumberOfThreads,
    _In_ KTHREAD_POOL_SYNC_TYPE SyncType,
    _Inout_ KTHREAD_POOL* ThreadPool
    )
{
    NTSTATUS status;

    if (0 == NumberOfThreads)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (NULL == ThreadPool)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    if (kthreadPoolUnknown != ThreadPool->ThreadPoolState)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    switch (SyncType)
    {
        case kthreadPoolSyncTypeSpinLock:
        {
            KeInitializeSpinLock(&ThreadPool->QueueSpinLock);
            break;
        }
        case kthreadPoolSyncTypeEresource:
        {
            status = ExInitializeResourceLite(&ThreadPool->QueueResource);
            if (!NT_SUCCESS(status))
            {
                Drv0LogError("[ERROR] ExInitializeResourceLite: 0x%08x", status);
                return status;
            }
            break;
        }
        case kthreadPoolSyncTypePushLock:
        {
            ExInitializePushLock(&ThreadPool->QueuePushLock);
            break;
        }
        default:
        {
            Drv0LogError("[ERROR] Unknown sync type: %d", SyncType);
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    ThreadPool->NumberOfThreads = NumberOfThreads;
    ThreadPool->TypeOfSynchronization = SyncType;

    KeInitializeEvent(&ThreadPool->QueueSignalEvent, NotificationEvent, FALSE);

    ThreadPool->Threads = ExAllocatePoolWithTag(NonPagedPool, NumberOfThreads * sizeof(HANDLE), TAG_THRPOOL_KTHR);
    if (NULL == ThreadPool->Threads)
    {
        Drv0LogError("[ERROR] ExAllocatePoolWithTag returned NULL");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&ThreadPool->WorkQueue);

    ThreadPool->ThreadPoolState = kthreadPoolStarted;

    for (DWORD i = 0; i < NumberOfThreads; i++)
    {
        status = PsCreateSystemThread(&ThreadPool->Threads[i],
            0,
            NULL,
            0,
            0,
            _KThrpDefaultThreadExecRoutine,
            ThreadPool);

        if (!NT_SUCCESS(status))
        {
            Drv0LogError("[ERROR] PsCreateSystemThread failed: 0x%08x\n", status);
            goto cleanup_and_exit;
        }
    }

cleanup_and_exit:
    if (!NT_SUCCESS(status))
    {
        ThreadPool->ThreadPoolState = kthreadPoolStopped;

        if (NULL != ThreadPool->Threads)
        {
            KeSetEvent(&ThreadPool->QueueSignalEvent, 0, TRUE);

            for (DWORD i = 0; i < NumberOfThreads; i++)
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
                        Drv0LogError("[ERROR] ObReferenceObjectByHandle failed: 0x%08x\n", status);
                        continue;
                    }

                    status = KeWaitForSingleObject(object, Executive, KernelMode, FALSE, NULL);
                    if (status != STATUS_SUCCESS)
                    {
                        Drv0LogWarning("[WARNING] KeWaitForSingleObject returned: 0x%08x\n", status);
                        status = STATUS_SUCCESS;
                    }

                    ZwClose(ThreadPool->Threads[i]);
                }
                
            }

            ExFreePoolWithTag(ThreadPool->Threads, TAG_THRPOOL_KTHR);
        }
    }

    return status;
}


//
// KThrpEnqueueWorkInThreadPool
//
NTSTATUS
KThrpEnqueueWorkInThreadPool(
    _In_ KTHREAD_POOL* ThreadPool,
    _In_ KTHREAD_POOL_WORK_ITEM* WorkItem
    )
{
    NTSTATUS status;
    KIRQL oldIrql;

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

    switch (ThreadPool->TypeOfSynchronization)
    {
        case kthreadPoolSyncTypeSpinLock:
        {
            KeAcquireSpinLock(&ThreadPool->QueueSpinLock, &oldIrql);
            break;
        }
        case kthreadPoolSyncTypeEresource:
        {
            ExEnterCriticalRegionAndAcquireResourceExclusive(&ThreadPool->QueueResource);
            break;
        }
        case kthreadPoolSyncTypePushLock:
        {
            ExAcquirePushLockExclusive(&ThreadPool->QueuePushLock);
            break;
        }
    }


    switch (ThreadPool->TypeOfSynchronization)
    {
        case kthreadPoolSyncTypeSpinLock:
        {
            KeReleaseSpinLock(&ThreadPool->QueueSpinLock, oldIrql);
            break;
        }
        case kthreadPoolSyncTypeEresource:
        {
            ExReleaseResourceAndLeaveCriticalRegion(&ThreadPool->QueueResource);
            break;
        }
        case kthreadPoolSyncTypePushLock:
        {
            ExReleasePushLockExclusive(&ThreadPool->QueuePushLock);
            break;
        }
    }
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
                    Drv0LogError("[ERROR] ObReferenceObjectByHandle failed: 0x%08x\n", status);
                    continue;
                }

                status = KeWaitForSingleObject(object, Executive, KernelMode, FALSE, NULL);
                if (status != STATUS_SUCCESS)
                {
                    Drv0LogWarning("[WARNING] KeWaitForSingleObject returned: 0x%08x\n", status);
                    status = STATUS_SUCCESS;
                }

                ZwClose(ThreadPool->Threads[i]);
            }

        }

        ExFreePoolWithTag(ThreadPool->Threads, TAG_THRPOOL_KTHR);
    }
}