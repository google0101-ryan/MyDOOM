#pragma once

class idCmdArgs
{
public:
    idCmdArgs() {argc = 0;}
    idCmdArgs(const char* text, bool keepAsStrings) {TokenizeString(text, keepAsStrings);}
    
    void TokenizeString(const char* text, bool keepAsStrings);
    void AppendArg(const char* text);
private:
    static const int MAX_COMMAND_ARGS = 64;
    static const int MAX_COMMAND_STRING = 2*1024;

    int argc;
    char* argv[MAX_COMMAND_ARGS];
    char tokenized[MAX_COMMAND_STRING];
};