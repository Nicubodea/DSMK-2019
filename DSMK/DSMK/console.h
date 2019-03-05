#pragma once

class __declspec(dllexport) CommandInterpreter
{
public:
    bool InterpretCommand(
        std::vector<std::string> argv
    );
};