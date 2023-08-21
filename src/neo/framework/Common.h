#pragma once

class idCommon
{
public:
    virtual void Init(int argc, const char** argv, const char* cmdline) = 0;

    virtual void Error(const char* msg, ...) = 0;
    virtual void Warning(const char* msg, ...) = 0;
};

extern idCommon* common;