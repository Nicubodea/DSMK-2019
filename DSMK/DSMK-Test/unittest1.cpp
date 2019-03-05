#include "stdafx.h"
#include "CppUnitTest.h"
#include "../DSMK/console.h"
#include "../DSMK/console.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace DSMKTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
        
		TEST_METHOD(TestMethod1)
		{
			// TODO: Your test code here
            char* argv1[] = { "help" };
            char* argv2[] = { "exit" };
            char* argv3[] = { "lol" };

            CommandInterpreter commandInt;
            Assert::AreEqual(commandInt.InterpretCommand(1, argv1), true);

            Assert::AreEqual(commandInt.InterpretCommand(1, argv2), true);

            Assert::AreEqual(commandInt.InterpretCommand(1, argv3), false);
		}

	};
}