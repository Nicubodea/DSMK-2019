
#include <iostream>
#include "console.h"
#include "trace.h"
#include "./x64/Debug/console.tmh"

int main()
{
    WPP_INIT_TRACING(NULL);
    std::cout << "hello world";
    int i = 0;
    while (i < 100)
    {
        AppLogTrace("%s", "hello world");
        Sleep(1000);
        i++;
    }

    WPP_CLEANUP();
}


bool CommandInterpreter::InterpretCommand(
    int argc,
    char* argv[]
)
{
    for (int i = 0; i < argc; i++)
    {
        if (!strncmp(argv[i], "help", sizeof("help")))
        {
            std::cout << "help was given";

            return true;
        }

        if (!strncmp(argv[i], "exit", sizeof("exit")))
        {
            std::cout << "exit was given";

            return true;
        }

        std::cout<<"unknown command!";

        return false;
    }

    return true;
}