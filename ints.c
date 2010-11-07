#if 0
void InsertI4(int x, unsigned char *buf);
int  ExtractI4(unsigned char *buf);
int  ExtractB12(unsigned char *buf);
void InsertB12(int x, unsigned char *buf);
int  ExtractB20(unsigned char *buf);
void InsertB20(int x, unsigned char *buf);
int  ExtractI2(unsigned char *buf);
void InsertI2(int x, unsigned char *buf);
#endif

#include <stdio.h>
#include "xing_header.h"

void print_buf(const unsigned char *buf, int len)
{
    int i;

    for (i=0; i<len; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    unsigned char buf[64];
    int x;
    int y;
    int i;
    int j;


    printf("testing InsertB2210/ExtractB2210 over range (1-4093, 0-1023)\n");
    for (j=0, i=1; j<1024; j++, i+=4) {
        memset(buf, 0, sizeof(buf));
        x = y = 0;
        InsertB2210(buf, i, j);
        ExtractB2210(buf, &x, &y);
        if (x != i) {
            printf("\n === ERROR: (x, i) %d != %d ===\n\n", x, i);
        }
        if (y != j) {
            printf("\n === ERROR: (y, j) %d != %d ===\n\n", x, i);
        }
    }
    printf("test completed (%d, %d)\n", x, y);

    printf("testing InsertB2210/ExtractB2210 over range (1-4092001, 0-1023)\n");
    for (j=0, i=1; j<1024; j++, i+=4000) {
        memset(buf, 0, sizeof(buf));
        x = y = 0;
        InsertB2210(buf, i, j);
        ExtractB2210(buf, &x, &y);
        if (x != i) {
            printf("\n === ERROR: (x, i) %d != %d ===\n\n", x, i);
        }
        if (y != j) {
            printf("\n === ERROR: (y, j) %d != %d ===\n\n", x, i);
        }
    }
    printf("test completed (%d, %d)\n", x, y);

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 1000, 500);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(1000, 500) = %d %d\n", x, y);
    if (x != 1000 || y != 500) {
        printf("\n === ERROR: (%d, %d) != (1000, 500) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB20(1000, buf);
    print_buf(buf, 4);

    memset(buf, 0, sizeof(buf));
    InsertB20(10000, buf);
    print_buf(buf, 4);

    memset(buf, 0, sizeof(buf));
    InsertB20(100000, buf);
    print_buf(buf, 4);

    memset(buf, 0, sizeof(buf));
    InsertB20(1000000, buf);
    print_buf(buf, 4);

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 17, 15);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(17, 15) = %d %d\n", x, y);
    if (x != 17 || y != 15) {
        printf("\n === ERROR: (%d, %d) != (17, 15) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 15, 17);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(15, 17) = %d %d\n", x, y);
    if (x != 15 || y != 17) {
        printf("\n === ERROR: (%d, %d) != (15, 17) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 1000, 255);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(1000, 255) = %d %d\n", x, y);
    if (x != 1000 || y != 255) {
        printf("\n === ERROR: (%d, %d) != (1000, 255) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 1000, 500);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(1000, 500) = %d %d\n", x, y);
    if (x != 1000 || y != 500) {
        printf("\n === ERROR: (%d, %d) != (1000, 500) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 10000, 600);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(10000, 600) = %d %d\n", x, y);
    if (x != 10000 || y != 600) {
        printf("\n === ERROR: (%d, %d) != (10000, 600) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 100000, 701);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(100000, 701) = %d %d\n", x, y);
    if (x != 100000 || y != 701) {
        printf("\n === ERROR: (%d, %d) != (100000, 701) ===\n\n", x, y);
    }

    memset(buf, 0, sizeof(buf));
    InsertB2210(buf, 1048573, 998);
    print_buf(buf, 4);

    ExtractB2210(buf, &x, &y);
    printf("Extract(1048573, 998) = %d %d\n", x, y);
    if (x != 1048573 || y != 998) {
        printf("\n === ERROR: (%d, %d) != (1048573, 998) ===\n\n", x, y);
    }
    return 0;
}
