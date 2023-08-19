#pragma once

class idCommon
{
public:
    virtual void Init(int argc, const char** argv, const char* cmdline) = 0;
};

extern idCommon* common;