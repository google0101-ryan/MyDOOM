#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "neo/sys/posix/posix_public.h"
#include "neo/framework/Common.h"

int main(int argc, const char** argv)
{
    Posix_EarlyInit();

    if (argc > 1)
        common->Init(argc-1, argv+1, NULL);
    else
        common->Init(0, NULL, NULL);

    return 0;
}