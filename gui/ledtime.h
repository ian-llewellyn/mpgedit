#ifndef _LEDTIME_H
#define _LEDTIME_H

#include <gtk/gtk.h>
#include "playctrl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define _LEDTIME_INTEGER_DIGITS_COUNT   10
#define _LEDTIME_CANONICAL_DIGITS_COUNT 10
#define _LEDTIME_SECONDS_DIGITS_COUNT   11
#define _LEDTIME_MINSEC_DIGITS_COUNT    12

typedef enum {
    LEDTIME_FORMAT_CANONICAL = 1, /* 999:59.999   */
    LEDTIME_FORMAT_SECONDS,       /* 3596459.999  */
    LEDTIME_FORMAT_MINSEC,        /* 59940:59.999 */
    LEDTIME_FORMAT_INTEGER,       /* 4000000000   */
} mpgedit_ledtime_format_e;

struct _mpgedit_ledtime_t;

typedef struct _mpgedit_ledtime_format_t {
    mpgedit_ledtime_format_e tag;

    void (*format)(
        struct _mpgedit_ledtime_t *,
        long, 
        long);

    GtkWidget *box;

    /* hhh:MM:ss.mmm */
    GtkWidget *canonical_digits[_LEDTIME_CANONICAL_DIGITS_COUNT];

    /* sssssss.mmm */
    GtkWidget *seconds_digits[_LEDTIME_SECONDS_DIGITS_COUNT];

    /* MMMMM:ss.mmm */
    GtkWidget *minsec_digits[_LEDTIME_MINSEC_DIGITS_COUNT];

    /* DDDDDDDDDD */
    GtkWidget *integer_digits[_LEDTIME_INTEGER_DIGITS_COUNT];
} mpgedit_ledtime_format_t;


typedef struct _mpgedit_ledtime_t {
    void (*settime)(
        struct _mpgedit_ledtime_t *self, 
        long sec, long msec);

    void (*setinteger)(
        struct _mpgedit_ledtime_t *self, 
        long value);

    GtkWidget *(*canontime_init)(
        struct _mpgedit_ledtime_t *self, 
        long sec, long msec);

    GtkWidget *(*minsectime_init)(
        struct _mpgedit_ledtime_t *self, 
        long sec, long msec);

    GtkWidget *(*secondtime_init)(
        struct _mpgedit_ledtime_t *self, 
        long sec, long msec);

    GtkWidget *(*integer_init)(
        struct _mpgedit_ledtime_t *self, 
        long value);

    int (*getformat)(
        struct _mpgedit_ledtime_t *self);

    GtkWidget *widget;
    GtkWidget *digits[11];
    GtkWidget *colon;
    GtkWidget *colon_hour;
    GtkWidget *colon_minute;
    GtkWidget *period;
    GtkWidget *period_second;
    GtkWidget *blank;
    mpgedit_ledtime_format_t *format[5];
    mpgedit_ledtime_format_t *formatp;
    long sec;
    long msec;
} mpgedit_ledtime_t;

mpgedit_ledtime_t *mpgedit_ledtime_new(void);

#endif
