#include <gtk/gtk.h>
#include "playctrl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ledtime.h"


#define _DIGIT_ZERO      "zero_led.xpm"
#define _DIGIT_ONE       "one_led.xpm"
#define _DIGIT_TWO       "two_led.xpm"
#define _DIGIT_THREE     "three_led.xpm"
#define _DIGIT_FOUR      "four_led.xpm"
#define _DIGIT_FIVE      "five_led.xpm"
#define _DIGIT_SIX       "six_led.xpm"
#define _DIGIT_SEVEN     "seven_led.xpm"
#define _DIGIT_EIGHT     "eight_led.xpm"
#define _DIGIT_NINE      "nine_led.xpm"
#define _DIGIT_COLON     "colon_led.xpm"
#define _DIGIT_COLONHOUR "colon_hour_led.xpm"
#define _DIGIT_COLONMIN  "colon_minute_led.xpm"
#define _DIGIT_PERIOD    "period_led.xpm"
#define _DIGIT_PERIODSEC "period_second_led.xpm"
#define _DIGIT_BLANKD    "blankdigit_led.xpm"
#define _DIGIT_BLANKP    "blankpunct_led.xpm"


static GtkWidget *_ledtime_lookup_digit(mpgedit_ledtime_t *self, char digit)
{
    int i;

    GtkWidget *widget = NULL;

    if (isdigit(digit)) {
        i = digit - '0';
        widget = self->digits[i];
    }
    else if (digit == ':') {
        widget = self->colon;
    }
    else if (digit == 'H') {
        widget = self->colon_hour;
    }
    else if (digit == 'M') {
        widget = self->colon_minute;
    }
    else if (digit == ' ') {
        /* Blank digit */
        widget = self->digits[10];
    }
    else if (digit == '_') {
        /* Blank Punctuation */
        widget = self->blank;
    }
    else if (digit == '.') {
        widget = self->period;
    }
    else if (digit == 'S') {
        widget = self->period_second;
    }
    return widget;
}


static GtkWidget *_ledtime_get_digit(mpgedit_ledtime_t *self, char digit)
{
    GtkWidget *widget = NULL;

    widget = _ledtime_lookup_digit(self, digit);
    if (widget) {
        widget = gtk_image_new_from_pixbuf(
                     gtk_image_get_pixbuf((GtkImage *) widget));
    }
    if (widget) {
        gtk_widget_show(widget);
    }
    return widget;
}


static void _ledtime_set_digit(mpgedit_ledtime_t *self,
                               GtkWidget **entry,
                               char digit)
{
    GtkWidget *widget = NULL;

    widget = _ledtime_lookup_digit(self, digit);
    if (widget) {
        gtk_image_set_from_pixbuf((GtkImage *) *entry,
                                  gtk_image_get_pixbuf((GtkImage *) widget));
    }
}


void _ledtime_format_integer(mpgedit_ledtime_t *self,
                             long value, long dummy)
{
    char str[32];
    char *cp;
    int  i = 0;

    sprintf(str, "%10ld", value);
    cp = str;
    while (*cp) {
        _ledtime_set_digit(self, &self->formatp->integer_digits[i], *cp);
        cp++;
        i++;
    }
}


void _ledtime_format_time_seconds(mpgedit_ledtime_t *self, 
                                  long sec, long msec)
{
    long seconds = sec;
    long milli   = msec;
    char str[32];
    char *cp;
    int len;
    int i;

    cp = str;
    if (seconds) {
        cp += sprintf(cp, "%ldS", seconds);
    }
    else {
        cp += sprintf(cp, "  _");
    }

    if (seconds == 0 && milli) {
        cp += sprintf(cp, "S");
    }

    if (milli) {
        sprintf(cp, "%03ld", milli);
        cp[3] = '\0';
    }
    else {
        cp += sprintf(cp, "000");
    }

    i = 0;
    len = _LEDTIME_SECONDS_DIGITS_COUNT - strlen(str);
    while (len > 0) {
        _ledtime_set_digit(self, &self->formatp->seconds_digits[i], ' ');
        len--;
        i++;
    }

    cp = str;
    while (*cp) {
        _ledtime_set_digit(self, &self->formatp->seconds_digits[i], *cp);
        cp++;
        i++;
    }
}


void _ledtime_format_time_minsec(mpgedit_ledtime_t *self, 
                                 long sec, long msec)
{
    long total_sec = sec;
    long minutes = 0;
    long seconds = 0;
    int milli = msec;
    char str[32];
    char *cp;
    int len;
    int i;

    minutes = total_sec / 60;
    seconds = total_sec % 60;

    cp = str;
    cp += sprintf(cp, "%5ldM", minutes);
    cp += sprintf(cp, "%02ldS", seconds);
    sprintf(cp, "%03d", milli);
    cp[3] = '\0';

    i   = 0;
    len = _LEDTIME_SECONDS_DIGITS_COUNT - strlen(str);
    while (len > 0) {
        _ledtime_set_digit(self, &self->formatp->minsec_digits[i++], ' ');
        len--;
    }

    cp = str;
    while (*cp) {
        _ledtime_set_digit(self, &self->formatp->minsec_digits[i++], *cp);
        cp++;
    }
}


static void _ledtime_format_time_canonical(mpgedit_ledtime_t *self,
                                           long sec, long msec)
{
    long total_sec = sec;
    int hours   = 0;
    int minutes = 0;
    int seconds = 0;
    int milli   = msec;
    char *cp;
    char str[32];
    int len;
    int i;

    hours   = total_sec / 3600;
    minutes = (total_sec - hours * 3600) / 60;
    seconds = total_sec % 60;

    cp = str;
    if (hours) {
        cp += sprintf(cp, "%3dH", hours);
    }
    else {
        cp += sprintf(cp, "   _");
    }

    if (minutes) {
        cp += sprintf(cp, "%02dM", minutes);
    }
    else {
        cp += sprintf(cp, "00M");
    }

    if (seconds) {
        cp += sprintf(cp, "%02dS", seconds);
    }
    else {
        cp += sprintf(cp, "00S");
    }

    if (milli) {
        sprintf(cp, "%03d", milli);
        cp[3] = '\0';
    }
    else {
        sprintf(cp, "000");
    }

    i = 0;
    len = _LEDTIME_CANONICAL_DIGITS_COUNT - strlen(str);
    while (len > 0) {
        _ledtime_set_digit(self, &self->formatp->canonical_digits[i], ' ');
        len--;
        i++;
    }

    cp = str;
    while (*cp) {
        _ledtime_set_digit(self, &self->formatp->canonical_digits[i], *cp);
        cp++;
        i++;
    }
}


static void _ledtime_settime(mpgedit_ledtime_t *self, long sec, long msec)
{
    if (!self->format) {
        return;
    }

    self->sec  = sec;
    self->msec = msec;
    self->formatp->format(self, sec, msec);
}



static void _ledtime_setinteger(mpgedit_ledtime_t *self, long value)
{
    if (!self->format) {
        return;
    }
    self->sec = value;
    self->formatp->format(self, value, 0);
}


static GtkWidget *_time_init(mpgedit_ledtime_t *self, long sec, long msec)
{
    GtkWidget                *box;
    GtkWidget                *retbox;
    mpgedit_ledtime_format_t *format;
    int                      i = 0;
    int                      cnt;
    GtkWidget                *digit;

    retbox = gtk_hbox_new(FALSE, 0);
    format = (mpgedit_ledtime_format_t *) 
        calloc(1, sizeof(mpgedit_ledtime_format_t));
    format->tag                          = LEDTIME_FORMAT_SECONDS;
    format->format                       = _ledtime_format_time_seconds;
    box                                  = gtk_hbox_new(FALSE, 0);
    format->box                          = box;
    self->format[format->tag]            = format;

    /* Format: SSSSSSS.mmm; initial 0.000 */
    i = 0;
    cnt = _LEDTIME_SECONDS_DIGITS_COUNT - 5;
    while (cnt > 0) {
        digit = _ledtime_get_digit(self, ' ');
        format->seconds_digits[i++] = digit;
        gtk_widget_show(digit);
        gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
        cnt--;
    }
    digit = _ledtime_get_digit(self, '0');
    format->seconds_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, '.');
    format->seconds_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    cnt = 3;
    while (cnt > 0) {
        digit = _ledtime_get_digit(self, ' ');
        format->seconds_digits[i++] = digit;
        gtk_widget_show(digit);
        gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
        cnt--;
    }
    gtk_box_pack_start(GTK_BOX(retbox), box, FALSE, FALSE, 0);

    format = (mpgedit_ledtime_format_t *) 
        calloc(1, sizeof(mpgedit_ledtime_format_t));
    format->tag                          = LEDTIME_FORMAT_MINSEC;
    format->format                       = _ledtime_format_time_minsec;
    box                                  = gtk_hbox_new(FALSE, 0);
    format->box                          = box;
    self->format[format->tag]            = format;
    /* Format: MMMMM:ss.mmm; initial 0.000 */

    i   = 0;
    cnt = 5;
    while (cnt > 0) {
        digit = _ledtime_get_digit(self, ' ');
        format->minsec_digits[i++] = digit;
        gtk_widget_show(digit);
        gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
        cnt--;
    }
    digit = _ledtime_get_digit(self, ':');
    format->minsec_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, '0');
    format->minsec_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, '0');
    format->minsec_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, '.');
    format->minsec_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    cnt = 3; /* adam/TBD: why is this not 3?? */
    while (cnt > 0) {
        digit = _ledtime_get_digit(self, '0');
        format->minsec_digits[i++] = digit;
        gtk_widget_show(digit);
        gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
        cnt--;
    }
    gtk_box_pack_start(GTK_BOX(retbox), box, FALSE, FALSE, 0);

    format = (mpgedit_ledtime_format_t *) 
        calloc(1, sizeof(mpgedit_ledtime_format_t));
    format->tag                            = LEDTIME_FORMAT_CANONICAL;
    format->format                         = _ledtime_format_time_canonical;
    box                                    = gtk_hbox_new(FALSE, 0);
    format->box                            = box;
    self->format[format->tag]              = format;

    /* Format: hhh:mm:ss.mmm */
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[0] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[1] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[2] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, '_');
    format->canonical_digits[3] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[4] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[5] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    
    digit = _ledtime_get_digit(self, '_');
    format->canonical_digits[6] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[7] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[8] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    
    digit = _ledtime_get_digit(self, '_');
    format->canonical_digits[9] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);

    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[10] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[11] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    digit = _ledtime_get_digit(self, ' ');
    format->canonical_digits[12] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(retbox), box, FALSE, FALSE, 0);


    /* Digits format */
    format = (mpgedit_ledtime_format_t *) 
        calloc(1, sizeof(mpgedit_ledtime_format_t));
    format->tag                          = LEDTIME_FORMAT_INTEGER;
    format->format                       = _ledtime_format_integer;
    box                                  = gtk_hbox_new(FALSE, 0);
    format->box                          = box;
    self->format[format->tag]            = format;

    /* Format: DDDDDDDDDD; initial value 0 */
    i = 0;
    cnt = _LEDTIME_INTEGER_DIGITS_COUNT - 1;
    while (cnt > 0) {
        digit = _ledtime_get_digit(self, ' ');
        format->integer_digits[i++] = digit;
        gtk_widget_show(digit);
        gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
        cnt--;
    }
    digit = _ledtime_get_digit(self, '0');
    format->integer_digits[i++] = digit;
    gtk_widget_show(digit);
    gtk_box_pack_start(GTK_BOX(box), digit, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(retbox), box, FALSE, FALSE, 0);

    self->widget = retbox;
    retbox       = gtk_hbox_new(FALSE, 0);
    return retbox;
}


static void _hideall_times(mpgedit_ledtime_t *self)
{
    int i;

    for (i=0; i<sizeof(self->format)/sizeof(GtkWidget *); i++) {
        if (self->format[i]) {
            gtk_widget_hide (self->format[i]->box);
        }
    }
}


static GtkWidget *_secondtime_init(mpgedit_ledtime_t *self, long sec, long msec)
{
    _hideall_times(self);
    self->formatp = self->format[LEDTIME_FORMAT_SECONDS];
    self->formatp->format(self, self->sec, self->msec);
    gtk_widget_show(self->formatp->box);
    return self->widget;
}


static GtkWidget *_minsectime_init(mpgedit_ledtime_t *self, long sec, long msec)
{
    _hideall_times(self);
    self->formatp = self->format[LEDTIME_FORMAT_MINSEC];
    self->formatp->format(self, self->sec, self->msec);
    gtk_widget_show(self->formatp->box);
    return self->widget;
}


static GtkWidget *_canontime_init(mpgedit_ledtime_t *self, long sec, long msec)
{
    _hideall_times(self);
    self->formatp = self->format[LEDTIME_FORMAT_CANONICAL];
    self->formatp->format(self, self->sec, self->msec);
    gtk_widget_show(self->formatp->box);
    return self->widget;
}


static GtkWidget *_integer_init(mpgedit_ledtime_t *self, long value)
{
    _hideall_times(self);
    self->formatp = self->format[LEDTIME_FORMAT_INTEGER];
    self->formatp->format(self, value, 0);
    gtk_widget_show(self->formatp->box);
    return self->widget;
}


static int _getformat(mpgedit_ledtime_t *self)
{
    return (int) self->formatp->tag;
}


mpgedit_ledtime_t *mpgedit_ledtime_new(void)
{
    mpgedit_ledtime_t *ledtime;
    GtkSettings *settings = gtk_settings_get_default();

    ledtime = (mpgedit_ledtime_t *) calloc(1, sizeof(mpgedit_ledtime_t));
    if (!ledtime) {
        return NULL;
    }

    /*
     * Initialize the digits array
     */
    ledtime->digits[0]  = xpm_pixmap(settings, _DIGIT_ZERO);
    ledtime->digits[1]  = xpm_pixmap(settings, _DIGIT_ONE);
    ledtime->digits[2]  = xpm_pixmap(settings, _DIGIT_TWO);
    ledtime->digits[3]  = xpm_pixmap(settings, _DIGIT_THREE);
    ledtime->digits[4]  = xpm_pixmap(settings, _DIGIT_FOUR);
    ledtime->digits[5]  = xpm_pixmap(settings, _DIGIT_FIVE);
    ledtime->digits[6]  = xpm_pixmap(settings, _DIGIT_SIX);
    ledtime->digits[7]  = xpm_pixmap(settings, _DIGIT_SEVEN);
    ledtime->digits[8]  = xpm_pixmap(settings, _DIGIT_EIGHT);
    ledtime->digits[9]  = xpm_pixmap(settings, _DIGIT_NINE);
    ledtime->digits[10] = xpm_pixmap(settings, _DIGIT_BLANKD);

    ledtime->period        = xpm_pixmap(settings, _DIGIT_PERIOD);
    ledtime->period_second = xpm_pixmap(settings, _DIGIT_PERIODSEC);
    ledtime->colon         = xpm_pixmap(settings, _DIGIT_COLON);
    ledtime->colon_hour    = xpm_pixmap(settings, _DIGIT_COLONHOUR);
    ledtime->colon_minute  = xpm_pixmap(settings, _DIGIT_COLONMIN);
    ledtime->blank         = xpm_pixmap(settings, _DIGIT_BLANKP);

    /* Set method functions */
    ledtime->settime         = _ledtime_settime;
    ledtime->setinteger      = _ledtime_setinteger;
    ledtime->canontime_init  = _canontime_init;
    ledtime->secondtime_init = _secondtime_init;
    ledtime->minsectime_init = _minsectime_init;
    ledtime->integer_init    = _integer_init;
    ledtime->getformat       = _getformat;

    _time_init(ledtime, 0, 0);
    return ledtime;
}


#ifdef _UNIT_TEST
typedef struct _digit_box {
    mpgedit_ledtime_t *leds;
    long sec;
    long msec;
} digit_box;
 

gboolean idle(gpointer data)
{
    digit_box *dbox = (digit_box *) data;

    dbox->sec++;
    dbox->leds->settime(dbox->leds, dbox->sec, dbox->msec);
    /* Bail at 10000 for valgrind analysis */
    if (dbox->sec == 10000) {
        exit(0);
    }
    
    return TRUE;
}


int main(int argc, char *argv[])
{
    mpgedit_ledtime_t *leds;
    GtkWidget         *window;
    GtkWidget         *widget;
    char              *timestr;
    char              *argv1;
    long              sec = 0;
    long              msec = 0;
    char              *cp;
    digit_box         dbox;

    if (argc == 1) {
        printf("usage: %s timestring\n", argv[0]);
       return 1;
    }

    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    timestr = strdup(argv[1]);
    leds   = mpgedit_ledtime_new();
    if (argv[1][0] == '#') {
        widget = leds->integer_init(leds, 0);
        argv1  = &argv[1][1];
    }
    else if (argv[1][0] == '+') {
        widget = leds->canontime_init(leds, 0, 0);
        argv1  = &argv[1][1];
    }
    else if (argv[1][0] == '-') {
        widget = leds->minsectime_init(leds, 0, 0);
        argv1  = &argv[1][1];
    }
    else {
        widget = leds->secondtime_init(leds, 0, 0);
        argv1  = argv[1];
    }
    gtk_widget_show(widget);
    gtk_container_add(GTK_CONTAINER(window), widget);

    sec = strtol(argv1, &cp, 10);
    if (cp) {
        msec = strtol(cp+1, NULL, 10);
    }
    leds->settime(leds, sec, msec);
    memset(&dbox, 0, sizeof(dbox));
    dbox.leds = leds;
    dbox.sec = sec;
    dbox.msec = msec;
    gtk_timeout_add(100, idle, &dbox);

    gtk_widget_show(window);
    gtk_main();
    return 0;
}
#endif
