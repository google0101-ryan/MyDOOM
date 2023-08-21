#pragma once

class idCmdArgs
{
public:
    idCmdArgs() {argc = 0;}
    idCmdArgs(const char* text, bool keepAsStrings) {TokenizeString(text, keepAsStrings);}
    
    void TokenizeString(const char* text, bool keepAsStrings);
    void AppendArg(const char* text);

    const char* Args(int start, int end, bool escapeArgs) const;

    int Argc() const {return argc;}
    const char *			Argv( int arg ) const { return ( arg >= 0 && arg < argc ) ? argv[arg] : ""; }
private:
    static const int MAX_COMMAND_ARGS = 64;
    static const int MAX_COMMAND_STRING = 2*1024;

    int argc;
    char* argv[MAX_COMMAND_ARGS];
    char tokenized[MAX_COMMAND_STRING];
};