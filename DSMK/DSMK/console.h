#pragma once

#include "threadpool.h"

class CommandInterpreter
{
private:
    ThreadPool *tp;
public:
    CommandInterpreter()
    {
        tp = new ThreadPool(5);
    }

    ~CommandInterpreter()
    {
        delete tp;
    }

    bool InterpretCommand(
        std::vector<std::string> argv
    );
};