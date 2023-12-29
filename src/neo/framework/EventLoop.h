#pragma once

#include "sys/sys_public.h"
#include "CvarSystem.h"
#include "Filesystem.h"

const int MAX_PUSHED_EVENTS = 64;

class idEventLoop
{
public:
    idEventLoop();
    ~idEventLoop() {}

    void Init();

    idFile* com_journalFile;
    idFile* com_journalDataFile;
private:
    int initialTimeOffset;

    int com_pushedEventsHead, com_pushedEventsTail;
    sysEvent_t com_pushedEvents[MAX_PUSHED_EVENTS];

    static idCvar com_journal;
};

extern idEventLoop* eventLoop;