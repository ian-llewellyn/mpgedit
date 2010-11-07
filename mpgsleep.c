#if !defined(WIN32)

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int mpgedit_usleep(int usec)
{
    struct timeval tv;
    int sts;

    tv.tv_sec  = 0;
    tv.tv_usec = usec;

    sts = select(0, 0, 0, 0, &tv);
    return sts;
}

#else
#include "portability.h"

int mpgedit_usleep(int usec)
{
    _sleep(usec/1000);
    return 0;
}

#endif





#ifdef _UNIT_TEST
int main(int argc, char *argv[])
{
    mpgedit_usleep(900000);
    return 0;
}
#endif
