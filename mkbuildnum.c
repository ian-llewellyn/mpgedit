#include <time.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    time_t t;
    struct tm *tm_time;

    time(&t);
    tm_time = localtime(&t);
    printf("#define MPGEDIT_BUILDNUM \"%d-%03d-%02d%02d\"\n", 
           tm_time->tm_year + 1900,
           tm_time->tm_yday + 1, /* Jan 1 is 0, 366 on Dec 31 on leap year */
           tm_time->tm_hour,
           tm_time->tm_min);
    if (argc > 1) {
        printf("#define MPGEDIT_BUILDOS \"%s\"\n", argv[1]);
    }
    return 0;
}
