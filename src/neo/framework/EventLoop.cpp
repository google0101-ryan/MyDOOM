#include "EventLoop.h"

idCvar idEventLoop::com_journal("com_journal", "0", CVAR_INIT | CVAR_SYSTEM, "1 = record journal, 2 = playback journal");

idEventLoop eventLoopLocal;
idEventLoop* eventLoop = &eventLoopLocal;

idEventLoop::idEventLoop()
{
    com_journalFile = NULL;
    com_journalDataFile = NULL;
    initialTimeOffset = 0;
}

void idEventLoop::Init()
{
    initialTimeOffset = Sys_Milliseconds();

    common->StartupVariable("journal");

    if (com_journal.GetIntegerValue() == 1)
    {
        printf("Journaling events\n");
        com_journalFile = fileSystem->OpenFileWrite("journal.dat");
        com_journalDataFile = fileSystem->OpenFileWrite("journaldata.dat");
    }
    else if (com_journal.GetIntegerValue() == 2)
    {
        printf("Replaying journaled events\n");
        com_journalFile = fileSystem->OpenFileRead("journal.dat");
        com_journalDataFile = fileSystem->OpenFileRead("journaldata.dat");
    }

    if (!com_journalFile || !com_journalDataFile)
    {
        com_journal.SetValue("0");
        com_journalFile = NULL;
        com_journalDataFile = NULL;
        printf("Couldn't open journal file\n");
    }
}
