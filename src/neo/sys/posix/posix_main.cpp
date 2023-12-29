#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "neo/sys/posix/posix_public.h"

#define MAX_OSPATH 256
#define COMMAND_HISTORY 64

static termios tty_tc;

xthreadInfo asyncThread;

xthreadInfo* g_threads[10];

const int MAX_LOCAL_CRITICAL_SECTIONS = 5;
static pthread_mutex_t global_lock[ MAX_LOCAL_CRITICAL_SECTIONS ];

pthread_cond_t	event_cond[ 4 ];
bool			signaled[ 4 ];
bool			waiting[ 4 ];

void Posix_InitPThreads( ) {
	int i;
	pthread_mutexattr_t attr;

	// init critical sections
	for ( i = 0; i < MAX_LOCAL_CRITICAL_SECTIONS; i++ ) {
		pthread_mutexattr_init( &attr );
		pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
		pthread_mutex_init( &global_lock[i], &attr );
		pthread_mutexattr_destroy( &attr );
	}

	// init event sleep/triggers
	for ( i = 0; i < 4; i++ ) {
		pthread_cond_init( &event_cond[ i ], NULL );
		signaled[i] = false;
		waiting[i] = false;
	}

	// init threads table
	for ( i = 0; i < 10; i++ ) {
		g_threads[ i ] = NULL;
	}	
}

static int set_exit = 0;
static char exit_spawn[1024];

const int siglist[] = {
	SIGHUP,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGIOT,
	SIGBUS,
	SIGFPE,
	SIGSEGV,
	SIGPIPE,
	SIGABRT,
	//	SIGTTIN,
	//	SIGTTOU,
	-1
	};

const char *signames[] = {
	"SIGHUP",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGIOT",
	"SIGBUS",
	"SIGFPE",
	"SIGSEGV",
	"SIGPIPE",
	"SIGABRT",
	//	"SIGTTIN",
	//	"SIGTTOUT"
};

static char fatalError[ 1024 ];

static void sig_handler(int signum, siginfo_t* info, void* context)
{
    static bool double_fault = false;

    if (double_fault)
    {
        printf("double fault %s, bailing out\n", strsignal(signum));
        exit(-1);
    }

    double_fault = true;

    printf("singal caught: %s\nsi_code %d\n", strsignal(signum), info->si_code);

    if (fatalError[0])
        printf("Was in fatal error shutdown: %s\n", fatalError);
    
    printf("Trying to exit gracefully...\n");
    exit(signum);
}

void Sys_FPE_Handler(int signum, siginfo_t* info, void* context)
{
    printf("FPE\n");
}

void Posix_InitSigs()
{
    struct sigaction action;
    int i;

    fatalError[0] = '\0';

    action.sa_sigaction = sig_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_NODEFER;

    i = 0;
    while (siglist[i] != -1)
    {
        if (siglist[i] == SIGFPE)
        {
            action.sa_sigaction = Sys_FPE_Handler;
            if (sigaction(siglist[i], &action, NULL) != 0)
                printf("Failed to set SIGFPE handler: %s\n", strerror(errno));
            action.sa_sigaction = sig_handler;
        }
        else if (sigaction(siglist[i], &action, NULL) != 0)
            printf("Failed to set %s handler: %s\n", signames[i], strerror(errno));
        i++;
    }

    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

unsigned long sys_timeBase = 0;

int Sys_Milliseconds(void)
{
    int curtime;
    struct timeval tp;

    gettimeofday(&tp, NULL);

    if (!sys_timeBase)
    {
        sys_timeBase = tp.tv_sec;
        return tp.tv_usec / 1000;
    }

    curtime = (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;

    return curtime;
}

void Posix_EarlyInit(void)
{
    memset(&asyncThread, 0, sizeof(asyncThread));
    exit_spawn[0] = '\0';
    Posix_InitSigs();
    Sys_Milliseconds();
    Posix_InitPThreads();
}