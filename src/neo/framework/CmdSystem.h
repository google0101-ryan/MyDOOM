#pragma once

#include "idlib/precompiled.h"

typedef enum
{
    CMD_FL_ALL = -1,
    CMD_FL_CHEAT = BIT(0),
    CMD_FL_SYSTEM = BIT(1),
    CMD_FL_RENDERER = BIT(2),
    CMD_FL_SOUND = BIT(3),
    CMD_FL_GAME = BIT(4),
    CMD_FL_TOOL = BIT(5)
} cmdFlags_t;

typedef enum
{
    CMD_EXEC_NOW,
    CMD_EXEC_INSERT,
    CMD_EXEC_APPEND
} cmdExecution_t;

typedef void (*cmdFunction_t)(const idCmdArgs& args);
typedef void (*argCompletion_t)(const idCmdArgs& args);

class idCmdSystem
{
public:
    virtual void Init() = 0;
    virtual void AddCommand(const char *cmdName, cmdFunction_t function, int flags, const char *description) = 0;
};

extern idCmdSystem* cmdSystem;