#pragma once

class __declspec(dllexport) CommandInterpreter
{
public:
    bool InterpretCommand(
        int argc,
        char* argv[]
    );
};