#include "stdafx.h"
#include "CppUnitTest.h"
#include "../DSMK/threadpool.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void* DummyWorkFunction(void* DummyParam)
{
    int* dummy = (int*)DummyParam;

    *dummy += 1;

    return DummyParam;
}

namespace DSMKTest
{		
	TEST_CLASS(ThreadPoolUnitTest)
	{
	public:
        
		TEST_METHOD(VerySimpleTestWithOneTask)
		{
            ThreadPool tp(5);
            int value = 1;
            int h;

            tp.StartThreadPool();

            h = tp.EnqueueWork((PFUNC_WorkFunction)DummyWorkFunction, &value);

            while (NULL == tp.GetEnqueuedWork(h));

            tp.StopThreadPool();

            Assert::AreEqual(2, value);

		}

        TEST_METHOD(VerySimpleTestWithOneTaskRepeatedThousandTimes)
        {
            for (int i = 0; i < 1000; i++)
            {
                ThreadPool tp(5);
                int value = 1;
                int h;

                tp.StartThreadPool();

                h = tp.EnqueueWork((PFUNC_WorkFunction)DummyWorkFunction, &value);

                while (NULL == tp.GetEnqueuedWork(h));

                tp.StopThreadPool();

                Assert::AreEqual(2, value);
            }

        }

	};
}