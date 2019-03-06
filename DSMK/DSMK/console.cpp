#include <iostream>

#include "trace.h"
#include "./x64/Debug/console.tmh"

#include <sstream>
#include <string>
#include <iterator>
#include <vector>

#include "console.h"

bool gStop = false;

int main()
{
    WPP_INIT_TRACING(NULL);

    CommandInterpreter interpreter;
    char buff[101];

    while (!gStop)
    {
        std::cout << ">";

        std::cin.getline(buff, 100);

        std::istringstream buf(buff);
        std::istream_iterator<std::string> beg(buf), end;

        std::vector<std::string> tokens(beg, end);

        std::cout << "Command line: " << buff << "\n";

        AppLogInfo("Command given: %s", buff);

        interpreter.InterpretCommand(tokens);        
    }

    WPP_CLEANUP();
}


bool CommandInterpreter::InterpretCommand(
    std::vector<std::string> argv
)
{
    int argc = argv.size();
    AppLogTrace("argc = %d", argc);

    for (int i = 0; i < argc; i++)
    {
        AppLogTrace("argv[%d] = %s", i, argv[i].c_str());

        if (argv[i].compare(0, sizeof("help"), "help") == 0)
        {
            std::cout << "The following commands are available: \n";

            std::cout << "help      - prints the help of the console\n";

            std::cout << "exit      - exits the console\n";

            return true;
        }

        if (argv[i].compare(0, sizeof("exit"), "exit") == 0)
        {
            std::cout << "exit was given\n";

            gStop = 1;

            return true;
        }

        if (argv[i].compare(0, sizeof("start"), "start") == 0)
        {
            std::cout << "starting thread pool\n";

            tp->StartThreadPool();

            return true;
        }

        if (argv[i].compare(0, sizeof("stop"), "stop") == 0)
        {
            std::cout << "starting thread pool\n";

            tp->StopThreadPool();

            return true;
        }

        std::cout<<"unknown command!\n";

        AppLogError("Unable to process command: %s", argv[i].c_str());

        return false;
    }

    return true;
}