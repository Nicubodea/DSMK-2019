#pragma once

#include <Windows.h>
#include <queue>
#include <utility>
#include <map>
#include <iostream>

typedef void* (*PFUNC_WorkFunction)(void* Param);


class ThreadPool 
{
private:

    HANDLE *Threads;
    HANDLE QueueEvent;
    HANDLE QueueMutex;
    int NumberOfThreads;
    int CurrentHandle;

    int ThreadPoolState;

    struct Work 
    {
        PFUNC_WorkFunction Function;
        void* Param;
        int Handle;
    };

    std::queue<Work> WorkQueue;
    std::map<int, void*> ResultsMap;

    
    // There is something very weird...
    // when passing the object as a parameter to ThreadFunction (having only one parameter), the disassembled code
    // seems to have two params: in RCX it has he function address and in RDX it has the actual object...
    // so I've decided to put two parameters (and now it works), but the disassembled code has now 3 params: in RCX the object, in
    // RDX the function address and in R8 again the object...
    // I will let this this way (because it works), until I can explain, it might have something to have with the fact
    // that it is a function member in a class, although searching on stack overflow about this seems to have a dead end...
    DWORD ThreadFunction(
        void* Function,
        void* Object
    )
    {
        ThreadPool* obj = (ThreadPool*)Object;
        DWORD waitResult;
        Work currentItem;

        while (true)
        {
            if (obj->ThreadPoolState == 0)
            {
                goto exit_thread_function;
            }

            waitResult = WaitForSingleObject(obj->QueueEvent, INFINITE);
            if (waitResult != STATUS_WAIT_0)
            {
                std::cerr << "ERROR: WaitForSingleObject returned: " << std::hex << waitResult << "\n";
                goto exit_thread_function;
            }

            waitResult = WaitForSingleObject(obj->QueueMutex, INFINITE);
            if (waitResult != STATUS_WAIT_0)
            {
                std::cerr << "ERROR: WaitForSingleObject returned: " << waitResult << "\n";
                goto exit_thread_function;
            }

            if (obj->WorkQueue.size() == 0)
            {
                if (!ResetEvent(obj->QueueEvent))
                {
                    std::cerr << "ERROR: ResetEvent failed: " << GetLastError() << "\n";
                    goto exit_thread_function;
                }

                if (!ReleaseMutex(obj->QueueMutex))
                {
                    std::cerr << "ERROR: ReleaseMutex failed: " << GetLastError() << "\n";
                    goto exit_thread_function;
                }
                continue;
            }

            currentItem = obj->WorkQueue.front();

            obj->WorkQueue.pop();

            if (!ReleaseMutex(obj->QueueMutex))
            {
                std::cerr << "ERROR: ReleaseMutex failed: " << GetLastError() << "\n";
                goto exit_thread_function;
            }

            obj->ResultsMap[currentItem.Handle] = currentItem.Function(currentItem.Param);
        }

    exit_thread_function:
        return 0;
    }

public:
    ThreadPool(int NumberOfThreads) 
    {
        this->Threads = new HANDLE[NumberOfThreads];
        this->NumberOfThreads = NumberOfThreads;
        this->ThreadPoolState = 0;
        this->CurrentHandle = 0;
    }

    ~ThreadPool()
    {
        delete this->Threads;
    }

    void StartThreadPool()
    {
        if (this->ThreadPoolState == 1)
        {
            std::cerr << "ERROR: Thread pool already started!";
            return;
        }

        this->ThreadPoolState = 1;
        this->CurrentHandle = 0;

        this->QueueEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        if (this->QueueEvent == NULL)
        {
            std::cerr << "ERROR: CreateEventA failed: " << GetLastError() << "\n";
            return;
        }

        this->QueueMutex = CreateMutexA(NULL, FALSE, NULL);
        if (this->QueueMutex == NULL)
        {
            std::cerr << "ERROR: CreateMutexA failed: " << GetLastError() << "\n";
            return;
        }

        for (int i = 0; i < this->NumberOfThreads; i++)
        {
            DWORD(__stdcall ThreadPool::* pFunc)(void*, void*) = &ThreadPool::ThreadFunction;

            this->Threads[i] = CreateThread(NULL,
                0,
                (LPTHREAD_START_ROUTINE)(void*&)pFunc,
                this,
                0,
                NULL);

            if (this->Threads[i] == NULL)
            {
                std::cerr << "ERROR: CreateThread failed: " << GetLastError() << "\n";
            }
        }
    }

    int EnqueueWork(PFUNC_WorkFunction Function, void* Param)
    {
        DWORD waitResult;
        int resultId = -1;
        Work work;

        if (this->CurrentHandle < 0)
        {
            std::cerr << "ERROR: CurrentHandle is " << this->CurrentHandle << "\n";
            goto exit_enqueue;
        }

        waitResult = WaitForSingleObject(QueueMutex, INFINITE);
        if (waitResult != STATUS_WAIT_0)
        {
            std::cerr << "ERROR: WaitForSingleObject returned: " << waitResult << "\n";
            goto exit_enqueue;
        }

        work.Function = Function;
        work.Param = Param;
        work.Handle = this->CurrentHandle;

        WorkQueue.push(work);

        resultId = this->CurrentHandle;
        this->CurrentHandle++;

        if (!ReleaseMutex(QueueMutex))
        {
            std::cerr << "ERROR: ReleaseMutex failed: " << GetLastError() << "\n";
        }

        SetEvent(QueueEvent);

    exit_enqueue:
        return resultId;
    }

    void* GetEnqueuedWork(int Handle)
    {
        std::map<int, void*>::iterator it;

        it = ResultsMap.find(Handle);
        if (it != ResultsMap.end())
        {
            return it->second;
        }

        return NULL;
    }

    void StopThreadPool()
    {
        DWORD waitResult;

        if (this->ThreadPoolState == 0)
        {
            std::cerr << "ERROR: Thread pool already started!";
            return;
        }

        this->ThreadPoolState = 0;

        SetEvent(QueueEvent);

        for (int i = 0; i < this->NumberOfThreads; i++)
        {
            if (this->Threads[i] == NULL)
            {
                continue;
            }

            waitResult = WaitForSingleObject(Threads[i], INFINITE);
            if (waitResult != STATUS_WAIT_0)
            {
                std::cerr << "ERROR: WaitForSingleObject returned: " << waitResult << "\n";
            }

            CloseHandle(Threads[i]);

            Threads[i] = NULL;
        }

        CloseHandle(QueueEvent);
        CloseHandle(QueueMutex);

        while (!WorkQueue.empty())
        {
            WorkQueue.pop();
        }

        ResultsMap.clear();
    }
};