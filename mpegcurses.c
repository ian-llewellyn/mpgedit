/*
 * mpgedit curses interface
 *
 * Copyright (C) 2001-2004 Adam Bernstein. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307  USA.
 */

#ifndef lint
static char SccsId[] = "$Id: mpegcurses.c,v 1.41 2005/11/30 07:34:23 number6 Exp $";
#endif

/*
 *
 * How keyboard I/O is performed by this layer is confusing, so here is
 * some documentation. The main "event" loop is the do-while loop in
 * main().  curs_read_time() drives everything.  curs_read_time() only
 * returns when one of the terminal characters is entered; 
 * see int_term_array[] in curs_read_time for the list.  The character
 * returned by curs_read_time() is processed by the switch statement in
 * main.  Listed below are the data read widgets:
 *
 *   curs_get_out_filename();
 *     |
 *     +--curs_read_fname();
 *          |
 *          +--curs_read_str();
 *
 *   curs_get_abandoned_filename();
 *     |
 *     +--curs_read_fname();
 *          |
 *          +--curs_read_str();
 *
 *   curs_read_time();
 *     |
 *     +--curs_read_integer();
 *
 * main()
 *   do-while() {
 *     |
 *     +--curs_read_time();
 *          |
 *          +--curs_read_integer();
 *
 *     case 'f'
 *     case 'Q'
 *      |
 *      +--curs_read_fname();
 *
 *     case 'n'
 *     case 'q'
 *     case 'e'
 *     case 'E'
 *      |
 *      +--curs_get_out_filename();
 *
 *     case 'L'
 *      |
 *      +--curs_get_abandoned_filename();
 *           |
 *           +--curs_read_fname();
 *   }
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

#define _USE_CURSES_H
#include "portability.h"
#include "mp3time.h"
#include "mp3_header.h"
#include "mpegcurses.h"
#include "mpegindx.h"
#include "volumeif.h"
#include "cursutil.h"
#include "segment.h"
#include "editif.h"
#include "parsename.h"


#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef KEY_ESC
#define KEY_ESC 27
#endif
#ifndef KEY_CTRL_L
#define KEY_CTRL_L 0xc
#endif
#ifndef KEY_CTRL_U
#define KEY_CTRL_U 0x15
#endif

/*
 * Define these for platforms that don't already define them.
 * In these cases, getch() will never return these values,
 * but they still need to be defined, as the code references them.
 */
#ifndef KEY_SHIFT_L
#define KEY_SHIFT_L KEY_MAX
#endif
#ifndef KEY_SHIFT_R
#define KEY_SHIFT_R KEY_MAX
#endif
#ifndef KEY_ALT_L
#define KEY_ALT_L KEY_MAX
#endif
#ifndef KEY_ALT_R
#define KEY_ALT_R KEY_MAX
#endif
#ifndef KEY_CONTROL_L
#define KEY_CONTROL_L KEY_MAX
#endif
#ifndef KEY_CONTROL_R
#define KEY_CONTROL_R KEY_MAX
#endif

#ifndef _ASCII_BACKSPACE
#define _ASCII_BACKSPACE 8
#endif
#define _ASCII_DEL 127
#define _ASCII_LF 0xa       /* Same as '\n' */
#define _ASCII_CR 0xd       /* pdcurses returns this for enter key */

#define _CURSOR_VISIBLE       1
#define _CURSOR_INVISIBLE     0

#define CURS_TIMEFIELD_WIDTH 27                /* 2 times time field width */
#define START_TIME_OFFSET    24
#define END_TIME_OFFSET      12
#define VOLUME_XPOS          74
#define LENGTH_XPOS          45
#define INPUTFILE_XPOS       5
#define MAXY(y) (y-4)

/*
 * curs_warn_printw defines
 */
#define NO_PRESS_RETURN 0
#define PRESS_RETURN 1

typedef struct _terminal_chars {
    int *values;
    int len;
} terminal_chars;

typedef struct _input_field {
    int    cols;
    int    value;
    int    curxpos;
    char   sep;
    int    zeropad;
    int    maxval;
    struct _input_field *rnext;
    struct _input_field *lnext;
} input_field;

typedef struct _input_fname {
    char name[MAXPATHLEN];
    int len;
} input_fname;


typedef struct _input_time {
    input_field min;
    input_field sec;
    input_field msec;
    struct _input_field *if_cursor;
} input_time;


typedef struct _edit_line {
    input_time stime;
    input_time etime;
    input_time *it_cursor;
    input_fname filename;
} edit_line;


typedef struct _edit_array {
    edit_line **el_lines;
    edit_line *el_cursor;
    edit_line save_editline;
    input_time *intime;
    int       max;
    int       alloc_max;
    int       i;
    int       start;
    int       already_loaded;
    int       dirty;
} edit_array;


typedef struct _delete_line_array {
    edit_line *el_lines[20];
    int i;
    int max;
} delete_line_array;

typedef struct _edit_window {
    WINDOW *win;
    int y;
    int x;
    int resize_x;
} edit_window;


typedef struct _index_ctx_t {
    WINDOW *win;
    char   *file;
} index_ctx_t;


char *curs_time2string(input_time *t, char *str);
char *curs_edit_line_format(edit_window *ewin, edit_line *e, char *buf);
void curs_edit_line_write(edit_window *ewin, char *buf);
void curs_warn_printw(WINDOW *output, int y, int x, int enter, char *fmt, ...);
void curs_edit_array_display(edit_window *ewin,
                             edit_array *earray,
                             WINDOW *output);


int find_terminal(terminal_chars *t, int c)
{
    int done = 0;
    int i;

    for (i=0; i < t->len; i++) {
        if (t->values[i] == c) {
            done = 1;
            break;
        }
    }
    return done;
}


typedef struct _curs_sigwinch_ctx {
    WINDOW      *output;
    WINDOW      *root;
    WINDOW      *autoedit;
    edit_window *ewin;
    edit_array  *earray;
    int         *y;
    int         *x;
    int         winsz_fd;
} curs_sigwinch_ctx;


/*
 * Unfortunately, this must be a global variable for the signal
 * handler function to have access to it.
 */
static curs_sigwinch_ctx sigwinch_ctx;

#if defined(__linux) 
/*
 * Danger, very not portable code here!
 */

#include <fcntl.h>
#include <signal.h>
#include <termio.h>



void curs_sigwinch_func(int sigval)
{
    struct winsize winsz;
    int            sts;
    int            diff;
    int            maxy;
    int            maxx;

    /*
     * Get the current window size from the tty
     */
    if (sigwinch_ctx.winsz_fd == -1) {
        sigwinch_ctx.winsz_fd = open("/dev/tty", O_RDONLY);
    }
    sts = ioctl(sigwinch_ctx.winsz_fd, TIOCGWINSZ, &winsz);
    if (sts == -1) {
        return;
    }
    *sigwinch_ctx.y = winsz.ws_row;
    *sigwinch_ctx.x = winsz.ws_col;

    /*
     * Save the difference in the x window size for cursor positioning.
     */
    getmaxyx(sigwinch_ctx.root, maxy, maxx);
    sigwinch_ctx.ewin->resize_x += winsz.ws_col - maxx;

    /* 
     * Update the root window title header. Must refresh
     * root window before modifying the windows on top of it.
     */
    resizeterm(winsz.ws_row, winsz.ws_col);
    move(0, 0);
    clrtoeol();
    mvprintw(0, 5, "Input file");
    mvprintw(0, winsz.ws_col-CURS_TIMEFIELD_WIDTH+1, "Start time    End time");
    refresh();

    /*
     * Reposition the error output window, and resize the the edit
     * window and error window.
     */
    mvwin(sigwinch_ctx.output,       winsz.ws_row-3, 0);
    wresize(sigwinch_ctx.ewin->win,  winsz.ws_row-4, winsz.ws_col);
    wresize(sigwinch_ctx.output, 3,  *sigwinch_ctx.x);
    if (sigwinch_ctx.autoedit) {
        wresize(sigwinch_ctx.autoedit,  0, (winsz.ws_col/2-40)+2);
    }

    getmaxyx(sigwinch_ctx.ewin->win, maxy, maxx);
    if (sigwinch_ctx.ewin->y > maxy) {
        /*
         * Cursor is now off the bottom of the screen.  Want to change 
         * viewport position so that the cursor is still just at the 
         * bottom of the screen.  Compute the difference in the number
         * of lines between the bottom of the window, and the cursor.
         * Change the window start display line by this difference,
         * and move the cursor 'y' position up the same amount.
         */
        diff = sigwinch_ctx.ewin->y - maxy + 1;
        sigwinch_ctx.earray->start += diff; 
        sigwinch_ctx.ewin->y       -= diff; 
    }

    /*
     * Repost the edit window data.
     */
    curs_edit_array_display(sigwinch_ctx.ewin,
                            sigwinch_ctx.earray,
                            sigwinch_ctx.output);

    /*
     * Fixup cursor position.  Must reference cursor x position from the
     * current edit line.  Don't forget to refresh the screen after this
     * position change.
     */
    wmove(sigwinch_ctx.ewin->win, sigwinch_ctx.ewin->y,
          sigwinch_ctx.earray->intime->if_cursor->curxpos +
              sigwinch_ctx.ewin->resize_x);
    wrefresh(sigwinch_ctx.ewin->win);
            
    signal(SIGWINCH, curs_sigwinch_func);
}


void curs_install_sigwinch(edit_window *ewin, edit_array *earray,
                           WINDOW *root, WINDOW *output, int *y, int *x)
{
    sigwinch_ctx.ewin      = ewin;
    sigwinch_ctx.earray    = earray;
    sigwinch_ctx.output    = output;
    sigwinch_ctx.root      = root;
    sigwinch_ctx.y         = y;
    sigwinch_ctx.x         = x;
    sigwinch_ctx.winsz_fd  = -1;

    signal(SIGWINCH, curs_sigwinch_func);
}


#else

void curs_install_sigwinch(edit_window *ewin, edit_array *earray,
                           WINDOW *root, WINDOW *output, int *y, int *x)
{
    /* stub for win32 */

    sigwinch_ctx.ewin      = ewin;
    sigwinch_ctx.earray    = earray;
    sigwinch_ctx.output    = output;
    sigwinch_ctx.root      = root;
    sigwinch_ctx.y         = y;
    sigwinch_ctx.x         = x;
    sigwinch_ctx.winsz_fd  = -1;
}

#endif


void curs_time_fixup(input_time *t)
{
    if (t->msec.value < 0) {
        t->msec.value = t->msec.maxval-1;
        t->sec.value--;
    }
    if (t->sec.value < 0) {
        t->sec.value = t->sec.maxval-1;
        t->min.value--;
    }
    if (t->min.value < 0) {
        t->min.value = 0;
        t->min.value = t->min.maxval-1;
    }

    if (t->msec.value >= t->msec.maxval) {
        t->msec.value = 0;
        t->sec.value++;
    }
    if (t->sec.value >= t->sec.maxval) {
        t->sec.value = 0;
        t->min.value++;
    }
    if (t->min.value >= t->min.maxval) {
        t->min.value = 0;
    }
}


void curs_clear_time(input_time *t)
{
    t->min.value = 0;
    t->sec.value = 0;
    t->msec.value = 0;
}


int curs_input_time_init(input_time *t, int col)
{
    int status = 1;

    memset(t, 0, sizeof(*t));

    t->min.cols  = 4;
    t->sec.cols  = 2;
    t->msec.cols = 3;

    t->min.curxpos  = col;
    t->sec.curxpos  = col + 3;
    t->msec.curxpos = col + 7;

    t->min.sep  = ':';
    t->sec.sep  = '.';
    t->msec.sep = ' ';

    t->min.maxval = 10000;
    t->sec.maxval = 60;
    t->msec.maxval = 1000;

    t->min.zeropad = 0;
    t->sec.zeropad = 1;
    t->msec.zeropad = 1;

    t->min.rnext  = &t->sec;
    t->sec.rnext  = &t->msec;
    t->msec.rnext = &t->min;

    t->min.lnext  = &t->msec;
    t->sec.lnext  = &t->min;
    t->msec.lnext = &t->sec;

    t->if_cursor  = &t->min;
    curs_time_fixup(t);
    return status;
}


int curs_input_fname_init(input_fname *f, int width)
{
    int status = 1;

    memset(f, 0, sizeof(*f));
    f->len = width;

    return status;
}


void curs_delete_line_array_init(delete_line_array *d)
{
    memset(d, 0, sizeof(*d));
    d->max = sizeof(d->el_lines)/sizeof(delete_line_array *);
}


edit_line *curs_edit_line_undelete(delete_line_array *d)
{
    edit_line *l = NULL;

    if (d->i > 0) {
        d->i--;
    }
    l = d->el_lines[d->i];
    d->el_lines[d->i] = NULL;
    return l;
}


void curs_edit_line_delete(edit_line *e, delete_line_array *d)
{
    int i;

    if (d) {
        if (d->i >= d->max) {
            free(d->el_lines[0]);
            d->i--;
            for (i=0; i < d->i; i++) {
                d->el_lines[i] = d->el_lines[i+1];
            }
        }
        d->el_lines[d->i++] = e;
    }
    else {
        free(e);
    }
}


void curs_edit_array_display(edit_window *ewin,
                             edit_array *earray,
                             WINDOW *output)
{
    int winy, indx;
    long file_sec, file_usec;
    int sts;
    int vol;

    /* TBD: compute width based upon window width */
    char fmtbuf[1024];

    wmove(ewin->win, 0, 0);
    wclrtobot(ewin->win);
    for (winy=0, indx=earray->start; indx<=earray->max; winy++, indx++) {
        mvwprintw(ewin->win, winy, 0, 
                  curs_edit_line_format(ewin, earray->el_lines[indx], fmtbuf));
    }
    wmove(ewin->win, ewin->y, 0);
    curs_edit_line_write(ewin,
                         curs_edit_line_format(ewin,
                                               earray->el_cursor,
                                               fmtbuf));
    wmove(output, 0, 0);
    wclrtobot(output);
    sts = curs_get_size_from_index(earray->el_cursor->filename.name,
                                   &file_sec, &file_usec);
    if (sts == 0) {
        mvwprintw(output, 2, LENGTH_XPOS, "Length: %d:%02d.%03d|%d.%03ds", 
                  file_sec/60, file_sec%60, file_usec/1000, 
                  file_sec, file_usec/1000);
    }

    /*
     * Display current volume setting
     */
    vol = curs_get_volume();
    if (vol != -1) {
        mvwprintw(output, 2, VOLUME_XPOS, "v:%3d", vol);
    }

    wrefresh(output);
    wrefresh(ewin->win);
}


/*
 * Format element values into canonical string format for 
 * display on screen.
 */
char *curs_edit_line_format(edit_window *ewin, edit_line *e, char *buf)
{
    char *cp;
    char *sp;
    int  len;
    int  pad;
    int  win_width;
    int  maxx;
    int  maxy;

    /* TBD: compute width based upon window width */
    char local_buf[1024];

    if (!ewin || !e || !buf) {
        return NULL;
    }

    if (buf) {
        cp = buf;
    }
    else {
        cp = local_buf;
    }
    len = strlen(e->filename.name);
    getmaxyx(ewin->win, maxy, maxx);
    win_width = maxx - CURS_TIMEFIELD_WIDTH;

    if (len > win_width) {
        sp = e->filename.name + (len - win_width);
    }
    else {
        sp = e->filename.name;
    }
    strcpy(cp, sp);

    pad = maxx - strlen(cp) - CURS_TIMEFIELD_WIDTH;
    while (pad > 0) {
        strcat(cp, " ");
        pad--;
    }
    cp = cp + strlen(cp);

    curs_time_fixup(&e->stime);
    curs_time2string(&e->stime, cp);
    strcat(cp, " ");

    cp = cp + strlen(cp);
    curs_time_fixup(&e->etime);
    curs_time2string(&e->etime, cp);
    strcat(cp, "    ");

    return buf ? buf : NULL;
}


void curs_edit_line_write(edit_window *ewin, char *buf)
{
    wmove(ewin->win, ewin->y, ewin->x);
    wclrtoeol(ewin->win);
    wattron(ewin->win, A_REVERSE);
    mvwprintw(ewin->win, ewin->y, ewin->x,  buf);
    wattroff(ewin->win, A_REVERSE);
    wrefresh(ewin->win);
}



int curs_edit_window_init(edit_window *ewin, int winy, int winx)
{

    int status = 1;

    memset(ewin, 0, sizeof(*ewin));
    ewin->win = newwin(winy-4, winx, 1, 0);
    if (!ewin->win) {
        status = 0;
    }
    keypad(ewin->win,    TRUE);
    scrollok(ewin->win,  FALSE);
    return status;
}


edit_line *curs_edit_line_init(int winx)
{
    int status = 1;
    edit_line *e;

    e = (edit_line *) calloc(1, sizeof(edit_line));
    if (!e) {
        return NULL;
    }

    status = curs_input_fname_init(&e->filename, winx - CURS_TIMEFIELD_WIDTH);
    if (!status) {
        goto clean_exit;
    }

    status = curs_input_time_init(&e->stime, winx-START_TIME_OFFSET);
    if (!status) {
        goto clean_exit;
    }
    e->it_cursor = &e->stime;

    status = curs_input_time_init(&e->etime, winx-END_TIME_OFFSET);
    if (!status) {
        goto clean_exit;
    }

    if (!status) {
        free(e);
        e = NULL;
    }
clean_exit:
    return e;
}


void curs_set_fname(input_fname *f, char *name)
{
    strcpy(f->name, name);
}


void curs_edit_line_set(edit_line *line,
                        char *name,
                        input_time *stime,
                        input_time *etime)
{
    if (name) {
        curs_set_fname(&line->filename, name);
    }
    if (stime) {
        line->stime.min.value  = stime->min.value;
        line->stime.sec.value  = stime->sec.value;
        line->stime.msec.value = stime->msec.value;
    }
    if (etime) {
        line->etime.min.value  = etime->min.value;
        line->etime.sec.value  = etime->sec.value;
        line->etime.msec.value = etime->msec.value;
    }
}


int curs_input_char_map(int input)
{
    switch (input) {
      case _ASCII_CR:
        input = _ASCII_LF;
        break;

      case _ASCII_BACKSPACE:
      case _ASCII_DEL:
        input = KEY_BACKSPACE;
        break;
    }
    return input;
}


void curs_display_str(WINDOW *win, int xpos, int ypos, int width, 
                      char *str, char **ret_head, char *cur, char *tail)
{
    char pbuf[1024];
    int len;
    char *head = *ret_head;
    char *ptr;

    width--;
    width -= xpos;

    if (!head) {
        if (strlen(str) <= (unsigned) width)  {
            head = str;
        }
        else {
            head = str + strlen(str) - width;
        }
    }
    if (cur < head) {
        head--;
        if (cur < str) {
            cur = str;
        }
        if (head < str) {
            head = str;
        }
    }
    else if (cur - head > width) {
        head++;
    }
    else if (tail - head < width && head > str) {
        head--;
    }

    pbuf[0] = '\0';
    strncat(pbuf, head, width);
    ptr = pbuf + strlen(pbuf);
    len = width - strlen(pbuf);
    while (len-- > 0) {
        *ptr++ = ' ';
    }
    *ptr = '\0';
    mvwprintw(win, ypos, xpos, "%s", pbuf);
    wmove(win, ypos, cur-head+xpos);
    curs_set(_CURSOR_VISIBLE);
    wrefresh(win);
    *ret_head = head;
}


/*
 * This is a pretty low level function that implements the string widget
 * I/O.  It is not advised anyone would call this directly.  Call
 * curs_read_fname() instead.
 */
static int curs_read_str(WINDOW *win,
                         char *str,
                         int ypos,
                         int xpos,
                         int win_width,
                         terminal_chars *t,
                         int *last)
{
    char  *cur;
    char  *tail;
    int  input;
    int  done = 0;
    char *head = NULL;
    int  esc_space = 0;

    cur = tail = str + strlen(str);
    curs_display_str(win, xpos, ypos, win_width, str, &head, cur, tail);
    do {
        input = curs_input_char_map(wgetch(win));
        if (input == KEY_BACKSPACE) {
            if (cur == tail) {
                if (cur > str) {
                    *--cur = '\0';
                }
                tail = cur;
            }
            else if (cur > str) {
                /* cur is between tail and beginning */
                memmove(cur-1, cur, strlen(cur) + 1);
                cur--;
                tail--;
            }
        }
        else if (input == KEY_LEFT) {
            if (cur > str) {
                cur--;
            }
        }
        else if (input == KEY_RIGHT) {
            cur++;
            if (cur > tail) {
                cur = tail;
            }
        }
        else if (input == KEY_CTRL_U) {
            cur   = tail = str;
            *cur  = '\0';
            head  = NULL;
        }
        else if (input == KEY_SHIFT_L   || input == KEY_SHIFT_R ||
                 input == KEY_UP        || input == KEY_DOWN    ||
                 input == KEY_ALT_L     || input == KEY_ALT_R   ||
                 input == KEY_CONTROL_L || input == KEY_CONTROL_L)
        {
            /* Ignore these key events on Win32 */
            ;
        }
        else if (input && isprint(input)) {
            if (cur == tail) {
                if (esc_space) {
                    *(cur-1) = input;
                    esc_space = 0;
                }
                else {
                    *cur++ = input;
                }
                *cur = '\0';
                tail = cur;
            }
            else {
                if (esc_space) {
                    *(cur-1) = input;
                    esc_space = 0;
                }
                else {
                    memmove(cur+1, cur, strlen(cur) + 1);
                    *cur++ = input;
                    tail++;
                }
            }
        }
        curs_display_str(win, xpos, ypos, win_width, str, &head, cur, tail);
        done = find_terminal(t, input);
    } while (!done);
    *last = input;

    /*
     * Trim white space from start and end of string
     */
    for (cur = str; isspace(*cur); cur++) 
        ;
    if (cur != str) {
        memmove(str, cur, strlen(cur));
    }
    str[strlen(cur)] = '\0';
    for (cur = str+strlen(str)-1; cur != str && isspace(*cur); cur--) {
        *cur = '\0';
    }
    return *last;
}


int curs_read_integer(edit_window *ewin,
                      edit_line *el,
                      input_field *inf,
                      terminal_chars *t,
                      int *last)
{
    char str[8];
    int  len  = 0;
    int  done = 0;
    int  input;
    int  refresh;

    /* TBD: compute width based upon window width */
    char fmtbuf[1024];

    str[0] = '\0';
    wmove(ewin->win, ewin->y, inf->curxpos + ewin->resize_x);
    wrefresh(ewin->win);
    do {
        refresh = 0;
        
        input = curs_input_char_map(wgetch(ewin->win));
        if (input < UCHAR_MAX && isdigit(input)) {
            if (len < inf->cols) {
                str[len++] = input;
                str[len]   = '\0';
            }
            else {
                len = 0;
                str[len++] = input;
                str[len]   = '\0';
            }
            if (atoi(str) >= inf->maxval) {
                len = 0;
                str[len]   = '\0';
            }
            refresh = 1;
        }
        else if (input == KEY_BACKSPACE) {
            if (len == 0) {
                str[len] = '\0';
            }
            else {
                str[--len] = '\0';
            }
            refresh = 1;
        }

        if (refresh) {
            inf->value = atoi(str);
            curs_edit_line_format(ewin, el, fmtbuf);
            curs_edit_line_write(ewin, fmtbuf);
            wmove(ewin->win, ewin->y, inf->curxpos + ewin->resize_x);
            wrefresh(ewin->win);
        }
        done = find_terminal(t, input);
    } while (!done);
    *last = input;
    return(strlen(str) == 0  ? -1 : atoi(str));
}


int curs_read_fname(WINDOW *win, int y, int x, int resize_x,
                    input_fname *f, char *prompt, terminal_chars *terminal)
{
    int last;
    terminal_chars int_terminal;
    int int_term_array[] = { '\n',  KEY_ESC };
    int xoffset = 0;
    
    wattron(win, A_REVERSE);
    mvwprintw(win, y, x, "%*s", f->len + resize_x, " ");

    if (prompt) {
        mvwprintw(win, y, x, prompt);
        wmove(win, y, strlen(prompt) + x);
        xoffset = strlen(prompt) + x;
    }
    wrefresh(win);

    if (terminal) {
        int_terminal = *terminal;
    }
    else {
        int_terminal.values = int_term_array;
        int_terminal.len    = sizeof(int_term_array)/sizeof(int);
    }

    last = curs_read_str(win, f->name,
                         y, xoffset, f->len + resize_x, &int_terminal, &last);
    wattroff(win, A_REVERSE);
    wrefresh(win);
    return last;
}


int curs_read_time(edit_window *ewin, edit_line *el, input_time *intime)
{
    int            value = 0;
    int            done = 0;
    int            last;
    input_field    *t;

    /* TBD: compute width based upon window width */
    char fmtbuf[1024];

    terminal_chars int_terminal;
    int            int_term_array[] = 
        { 'f', 'n', ' ', KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
          ',', '.', 'h', 'l', 'k', 'j', 'J', 'c', '\t',
          's', 'S', 'e', 'E', 'q', 'Q', 'L', 'o', 'O', 'p', 'P', 'D',
          'H', 'G', 'v', 'V', '?', KEY_CTRL_L, KEY_ESC, 'A', 'C'};

    int_terminal.values = int_term_array;
    int_terminal.len    = sizeof(int_term_array)/sizeof(int);

    if (intime->if_cursor) {
        t = intime->if_cursor;
    }
    else {
        t = intime->if_cursor = &intime->min;
    }

    do {
        value = curs_read_integer(ewin,
                                  el,
                                  t,
                                  &int_terminal, &last);
        
        if (value != -1) {
            t->value = value % t->maxval;
            curs_edit_line_format(ewin, el, fmtbuf);
            curs_edit_line_write(ewin, fmtbuf);
        }

        switch(last) {
          case '.':
            t->value++;
            curs_edit_line_format(ewin, el, fmtbuf);
            curs_edit_line_write(ewin, fmtbuf);
            break;

          case ',':
            t->value--;
            curs_edit_line_format(ewin, el, fmtbuf);
            curs_edit_line_write(ewin, fmtbuf);
            break;

          case KEY_RIGHT:
          case 'l':
          case ' ':
            if (t == &intime->msec) {
                done = 1;
            }
            else {
                t = intime->if_cursor = t->rnext;
            }
            break;

          case KEY_LEFT:
          case 'h':
            if (t == &intime->min) {
                done = 1;
            }
            else {
                t = intime->if_cursor = t->lnext;
            }
            break;

          case 'c':
            curs_clear_time(intime);
            t->value               = 0;
            t->rnext->value        = 0;
            t->rnext->rnext->value = 0;
            t                      = &intime->min;
            intime->if_cursor      = t;
            curs_edit_line_format(ewin, el, fmtbuf);
            curs_edit_line_write(ewin, fmtbuf);
            break;

          /*
           * Anything listed in int_term_array that is not listed in
           * this switch statement causes curs_read_time() to return to
           * the caller, who must handle the input character.
           */
          case KEY_UP:
          case KEY_DOWN:
          default:
            done = 1;
            break;
        }
        wmove(ewin->win, ewin->y, t->curxpos + ewin->resize_x);
        wrefresh(ewin->win);
    } while (!done);
    return last;
}


char *curs_time2string(input_time *t, char *str)
{
    sprintf(str, "%4d:%.2d.%.3d", t->min.value, t->sec.value, t->msec.value);
    return str;
}


void curs_string2time(char *str, input_time *t)
{
    char *cp;
    int val;

    val = strtol(str, &cp, 10); 
    t->min.value = val;
    cp++;

    val = strtol(cp, &cp, 10); 
    t->sec.value = val;
    cp++;
    
    val = strtol(cp, &cp, 10); 
    t->msec.value = val;
    cp++;
}


int curs_input_time_iszero(input_time *t) 
{
    return t->min.value == 0 && t->sec.value == 0 && t->msec.value == 0;
}



/*
 * Returns 0  if equal,
 *         <0 if t1 < t2
 *         >0 if t1 > t2
 *        -1 if t1 or t2 is NULL.
 * Nothing can be assumed by the numeric value returned, as it may
 * be the difference between seconds or milliseconds. 
 */
int curs_input_time_cmp(input_time *t1, input_time *t2)
{
    int sec1;
    int sec2;
    int msec1;
    int msec2;
    
    /* Not exactly correct, but must return something for this case */
    if (!t1 || !t2) {
        return -1;
    }

    sec1  = t1->min.value * 60 + t1->sec.value;
    sec2  = t2->min.value * 60 + t2->sec.value;
    msec1 = t1->msec.value;
    msec2 = t2->msec.value;

    /* Equality test */
    if (sec1 == sec2 && msec1 == msec2) {
        return 0;
    }

    /* Must differ only by milliseconds */
    if (sec1 == sec2) {
        return msec1-msec2;
    }

    /* Must differ by seconds */
    return sec1-sec2;
}



editspec_t *curs_build_editspec(edit_array *earray, int *status)
{
    int i;
    int maxlen;
    editspec_t *edits;
    char str[12];
    char timespec[32];
    int iszero1;
    int iszero2;
    int error = 0;


    maxlen = earray->max+1;
    edits = mpgedit_editspec_init();
    if (!edits) {
        return edits;
    }

    for (i=0; i<maxlen; i++) {
        iszero1 = curs_input_time_iszero(&earray->el_lines[i]->stime);
        iszero2 = curs_input_time_iszero(&earray->el_lines[i]->etime);
        timespec[0] = '\0';
        if (earray->el_lines[i]->filename.name &&
            *earray->el_lines[i]->filename.name)
        {
            /*
             * Build '-e-' specification when both values are zero.
             * This is a file copy of input to output.
             */
            if (iszero1 && iszero2) {
                strcat(timespec, "-");
                mpgedit_editspec_append(edits, 
                                        earray->el_lines[i]->filename.name,
                                        timespec);
            }
            else {
                /*
                 * Test the constraint that time2 must be greater than time1
                 */
                if (!iszero1 && !iszero2 &&
                    curs_input_time_cmp(&earray->el_lines[i]->etime,
                                        &earray->el_lines[i]->stime) <= 0)
                {
                        error = 1;
                }
                if (!iszero1) {
                    curs_time2string(&earray->el_lines[i]->stime, str);
                    strcat(timespec, str);
                }
                strcat(timespec, "-");
                if (!iszero2) {
                    curs_time2string(&earray->el_lines[i]->etime, str);
                    strcat(timespec, str);
                }
                mpgedit_editspec_append(edits,
                                        earray->el_lines[i]->filename.name,
                                        timespec);
            }
        }
        else {
            /*
             * Flag an error when no filename is filled in but there 
             * is edit specification.
             */ 
            error = 3;
        }
    }
    if (error) {
        mpgedit_editspec_free(edits);
    }
    if (status) {
        *status = error;
    }
    return edits;
}



int ttyio_getch(void *ctx)
{
    char input;

    input = getch();
    if (input == _ASCII_CR) {
        input = _ASCII_LF;
    }
    else if (input == 0) {
        input = -1;
    }
    return input;
}


static void ttyio_printf(void *ctx, int y, int x, const char *fmt, ...)
{
    /* TBD: compute width based upon window width */
    char buf[1024];
    WINDOW *win = (WINDOW *) ctx;

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    mvwprintw(win, y, x, buf);
    wrefresh(win);
    va_end(ap);
}


static int index_callback(void *data, long sec,
                          long msec, long frame, char *dummy)
{
    mpegfio_iocallbacks *ttyio = (mpegfio_iocallbacks *) data;
    index_ctx_t *ctx = (index_ctx_t *) ttyio->ctx;

    if (!data || !ttyio->printf) {
        return 1;
    }
    ttyio->printf(ctx->win, 2, 0,
                  "\rIndexing '%s': %4d:%02d| %5d.%03ds|Frame: %-7d\r",
                  ctx->file,
                  sec/60,
                  sec%60,
                  sec,
                  msec,
                  frame);
    return 1;
}


void curs_warn_printw(WINDOW *output, int y, int x, int enter, char *fmt, ...)
{
    char buf[1024];   /* Should be big enough  :< */
    va_list ap;

    flash();
    wattron(output, A_REVERSE);

    /*
     * No vwprintw() on Win32, so format it first
     */
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    mvwprintw(output, y, x, buf);
    if (enter == PRESS_RETURN) {
        wprintw(output, " <Press return>");
        wgetch(output);
    }

    wattroff(output, A_REVERSE);
    wrefresh(output);
}


int curs_call_decodefile(char *fname, cmdflags *argvflags, WINDOW *win)
{
    mpegfio_iocallbacks ttyio;
    int                 s_flag_save;

    memset(&ttyio, 0, sizeof(ttyio));
    ttyio.printf      = ttyio_printf;
    ttyio.ctx         = win;
    s_flag_save       = argvflags->s_flag;
    argvflags->s_flag = 1;
    _mpgedit_decode_file(fname, 0, 0,  MP3_TIME_INFINITE, 0, argvflags, &ttyio);
    argvflags->s_flag = s_flag_save;
    return 0;
}


int curs_call_indexfile(char *file, WINDOW *win)
{

    int                 status = 0;
    mpegfio_iocallbacks ttyio;
    index_ctx_t         indexctx;
    char                *ptr;

    memset(&ttyio, 0, sizeof(ttyio));

    indexctx.win  = win;

    /* 40 is assuming a terminal width of 80 */
    indexctx.file = (ptr = (file + strlen(file) - 30)) < file ? file : ptr;

    ttyio.getch  = ttyio_getch;
    ttyio.printf = ttyio_printf;
    ttyio.ctx    = &indexctx;

    /*
     * Move cursor to the output box area, just in case
     * the index file will be created.  Text put occurs.
     */
    wmove(win, 0, 0);
    wclrtoeol(win);
    wmove(win, 1, 0);
    if (curs_build_indexfile(file, index_callback, &ttyio)) {
        wmove(win, 0, 0);
        wclrtobot(win);

        curs_warn_printw(win, 2, 0,  PRESS_RETURN,
                         "Failed opening file '%s'", file);
        status = 1;
    }
    else {
        wmove(win, 0, 0);
        wclrtobot(win);
    }
    wrefresh(win);
    return status;
}


void curs_error_refresh(edit_window *ewin,
                        edit_array *earray,
                        WINDOW *output)
{
    wrefresh(output);
    wmove(output, 0, 0);
    wclrtobot(output);
    curs_edit_array_display(ewin, earray, output);
    wrefresh(output);
}


int curs_insert_edit_line(char *file,
                          char *start_time,
                          char *end_time,
                          int lineno,
                          edit_window *ewin,
                          edit_array *earray,
                          WINDOW *output,
                          int winx)
{
    edit_line **realloc_lines;

    /* Validate input file first */
    if (curs_call_indexfile(file, output)) {
        curs_edit_array_display(ewin, earray, output);
        return 1;
    }

    earray->i = lineno;
    if (lineno >= earray->alloc_max) {
        realloc_lines = (edit_line **)
            realloc(earray->el_lines,
                    sizeof(edit_line *) * earray->alloc_max * 2);
        if (!realloc_lines) {

            /* Not enough memory for operation */
            /* adam error message here */

            mvwprintw(output, 2, 0,
                      "Failed allocating more memory for edit buffer");
            wrefresh(output);
            return 1;
        }
        else {
            earray->el_lines = realloc_lines;
            earray->alloc_max *= 2;
        }
    }
    if (lineno > earray->max) {
        earray->el_lines[earray->i] = curs_edit_line_init(winx);
        if (!earray->el_lines[earray->i]) {
             fprintf(stderr, "%s: %s\n",
                     "curs_load_abandoned_edits",
                     "curs_edit_line_init failed");
             return 1;
        }
        earray->max = earray->i;
    }
    strcpy(earray->el_lines[lineno]->filename.name, file);
    curs_string2time(start_time, &earray->el_lines[lineno]->stime);
    curs_string2time(end_time, &earray->el_lines[lineno]->etime);
    curs_set_fname(&earray->el_lines[lineno]->filename, file);

    curs_edit_line_format(ewin, earray->el_lines[lineno], NULL);
    ewin->y++;
    return 0;
}


int curs_load_abandoned_edits(char        *file,
                              edit_window *ewin,
                              edit_array  *earray,
                              WINDOW      *output,
                              int         winx)
{
    FILE      *fp;
    int       line;
    char      inbuf[MAXPATHLEN+32];
    char      *namep;
    char      *stp;
    char      *etp;
    char      *cp;
    int       sts = 1;
    int       maxy;
    int       maxx;

    fp = fopen(file, "rb");
    if (!fp) {
        return 1;
    }
    
    line = 0;
    do {
        cp = fgets(inbuf, sizeof(inbuf)-1, fp);
        if (!cp) {
            /* Most likely EOF */
            continue;
        }
        
        inbuf[strlen(inbuf)-1] = '\0';
        namep = stp = etp = NULL;
        cp = strstr(inbuf, "%%");
        if (!cp) {
            goto clean_exit;
        }
        if (cp) {
            namep = inbuf;
            *cp = '\0';
            cp+=2;
            stp = cp;
        }
        if (cp < inbuf+sizeof(inbuf) && *cp) {
            cp = strstr(cp, "%%");
            if (cp) {
                *cp = '\0';
                cp+=2;
                etp = cp;
            }
        }
        if (namep && stp && etp) {
            sts = curs_insert_edit_line(namep, stp, etp, line,
                                        ewin, earray, output, winx);
            if (sts == 0) {
                line++;
            }
        }
    } while (!feof(fp));
    sts = 0;
    ewin->y--;

    /*
     * Position viewport on first screen of loaded data when
     * more el_lines were loaded than can be displayed.
     */
    getmaxyx(ewin->win, maxy, maxx);
    if (ewin->y >= maxy) {
        ewin->y = maxy - 1;
        earray->i = ewin->y;
    }
    earray->el_cursor = earray->el_lines[earray->i];
    curs_edit_array_display(ewin, earray, output);
    wmove(ewin->win, ewin->y,
          earray->el_cursor->stime.min.curxpos + ewin->resize_x);
    wrefresh(ewin->win);

clean_exit:
    fclose(fp);
    return sts;
}


int curs_save_abandoned_edits(edit_array *earray, char *filename)
{
    FILE *fp;
    int i;
    char stbuf[32];
    char etbuf[32];
    char *cp;

    fp = fopen(filename, "wb");
    if (!fp) {
        return 1;
    }
    for (i=0; i<=earray->max; i++) {
        cp = earray->el_lines[i]->filename.name;
        curs_time2string(&earray->el_lines[i]->stime, stbuf);
        curs_time2string(&earray->el_lines[i]->etime, etbuf);
        fprintf(fp, "%s%%%%%s%%%%%s\n", cp, stbuf, etbuf);
    }
    fclose(fp);
    return 0;
}


int curs_get_out_filename(input_fname *filename,
                          char *prompt,
                          char *errprompt,
                          WINDOW *win,
                          int resize_x,
                          int uniq)
{
    int status;
    int inchar;
    input_fname response;
    char *errstr = NULL;

    curs_input_fname_init(&response, 70);
    do {
        inchar = curs_read_fname(win, 2, 0, resize_x,
                                 filename, prompt, NULL);
        if (inchar == KEY_ESC) {
            status = 0;
            break;
        }
        status = validate_outfile(filename->name, 0, &errstr);
        if (status) {
            wmove(win, 0, 0);
            wclrtobot(win);
            if (status == 1) {
                if (!uniq) {
                    flash();
                    strcpy(response.name, "");
                    inchar = curs_read_fname(win, 2, 0,
                                             resize_x, &response, errprompt, NULL);
                    if (!response.name[0]) {
                        strcpy(response.name, "No");
                    }
                    if (response.name[0] == 'y' || response.name[0] == 'Y') {
                        status = 0;
                    }
                    else if (response.name[0] == 'n' ||
                             response.name[0] == 'N') 
                    {
                        inchar = KEY_ESC;
                        status = 0;
                    }
                    else if (inchar == KEY_ESC) {
                        status = 0;
                        break;
                    }
                }
                else {
                    curs_warn_printw(win, 1, 0, NO_PRESS_RETURN, errstr);
                }
            }
            else {
                curs_warn_printw(win, 1, 0, NO_PRESS_RETURN, errstr);
            }
        }
        else {
            wmove(win, 0, 0);
            wclrtobot(win);
        }
        if (errstr) {
            free(errstr);
        }
    } while (status);

    return inchar;
}


int curs_get_abandoned_filename(input_fname *filename,
                                int resize_x, WINDOW *win)
{
    int status = 0;
    int inchar;

    do {
        inchar = curs_read_fname(win, 2, 0, resize_x,
                                 filename, "Abandoned file: ", NULL);
        if (inchar == KEY_ESC) {
            break;
        }
        status = access(filename->name, F_OK);
        if (status == -1) {
            wmove(win, 0, 0);
            wclrtoeol(win);
            wmove(win, 1, 0);
            wclrtoeol(win);
            mvwprintw(win, 1, 0,
                      "File does not exist '%s'",
                       filename->name);
        }
        else {
            wmove(win, 0, 0);
            wclrtobot(win);
        }
        wrefresh(win);
    } while (status == -1 && inchar != '\n');
    return inchar;
}


char *curs_get_default_abandoned_filename(char *name)
{
    FILE  *fp;

    fp = fopen(".mp3edit_abandonded_name", "rb");
    if (!fp) {
        return NULL;
    }
    fgets(name, MAXPATHLEN, fp);
    fclose(fp);
    return name;
}


int curs_put_default_abandoned_filename(char *name)
{
    FILE *fp;
    int sts;

    fp = fopen(".mp3edit_abandonded_name", "wb");
    if (!fp) {
        return 1;
    }
    sts = fputs(name, fp);
    fclose(fp);
    return sts;
}


void curs_display_help(int maxy, int maxx)
{
    WINDOW *hwin;
    int    i;
    int    old_curs;
#   define HELP_WINDOW_DISPLAY_WIDTH   76
#   define HELP_WINDOW_FIELD_FMT       "%-73s"
#   define HELP_WINDOW_LINES           19
#   define HELP_WINDOW_COLUMNS         (HELP_WINDOW_DISPLAY_WIDTH-1)
#   define HELP_WINDOW_Y_POSITION      3
#   define HELP_WINDOW_X_POSITION      (((maxx/2)-40)+2)
#   define HELP_WINDOW_LEFT_MARGIN     1

    /* 
     * Center help window on screen.  Clear new window.
     */
    hwin = newwin(HELP_WINDOW_LINES,       HELP_WINDOW_COLUMNS, 
                  HELP_WINDOW_Y_POSITION,  HELP_WINDOW_X_POSITION);
    if (!hwin) {
        return;
    }
    wclrtobot(hwin);

    /*
     * Write in reverse video all blanks.  Afterwards, the contents
     * of the window are written in normal video.  This gives the illusion 
     * of a solid boarder around the text in the help window.  This is
     * a sleazy trick, but it is cheap and looks good.
     */
    wattron(hwin, A_REVERSE);
    for (i=0; i<maxy-6; i++) {
        mvwprintw(hwin, i, 0, "%*c", HELP_WINDOW_DISPLAY_WIDTH, ' ');
    }
    i=0;
    mvwprintw(hwin, i++, (HELP_WINDOW_DISPLAY_WIDTH/2) - 7, " mpgedit help ");
    wattroff(hwin, A_REVERSE);

    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|?| display help window             "
              "|A| autoedit files");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|c| zero selected time field        |,| time--    |.| time++");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|ESC| Undo last change              "
              "|^U| clear input text field");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|s| playback from time              "
              "|S| playback before time 5 seconds");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|e| write edits to named file       "
              "|E| write edits to default files");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|q| save current session and quit   "
              "|Q| discard current session");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|L| resume editing of saved session");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|f| input file name                 "
              "|n| output file name for 'e' command");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|o| open new line below cursor      "
              "|O| open new line above cursor");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|p| paste deleted line below cursor "
              "|P| paste deleted line above cursor");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|D| delete current edit line        |C| clear all edits");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|J| join next and current lines");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|v| Decrease playback volume        "
              "|V| Increase playback volume");

    wattron(hwin, A_REVERSE);
    mvwprintw(hwin, i++, (HELP_WINDOW_DISPLAY_WIDTH/2) - 10,
              "Cursor movement                 ");
    wattroff(hwin, A_REVERSE);

    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|j| move down     |k| move up       "
              "|h| move left    |l|  move right");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|H| move cursor to first line       "
              "|G| move cursor to last line");
    mvwprintw(hwin, i++, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "|TAB| toggle start/end time field   "
              "|space| move right, same as 'l'");
    wattron(hwin, A_REVERSE);
    mvwprintw(hwin, i, HELP_WINDOW_LEFT_MARGIN, HELP_WINDOW_FIELD_FMT,
              "Enter=close help");

    /*
     * Would like to make cursor invisible here, but curs_set(0) 
     * does not work everywhere, so move to a position that makes sense
     */
    old_curs = curs_set(_CURSOR_INVISIBLE);
    wattron(hwin, A_REVERSE);
    wmove(hwin, i, 1);
    wrefresh(hwin);
    getch();
    if (old_curs != ERR) {
        curs_set(old_curs);
    }
    delwin(hwin);
}


int curs_display_autoedit_menu(int maxy,
                               int maxx,
                               mpgedit_pcmlevel_t *levels,
                               char *input_file,
                               int *resize_x)
{
    WINDOW *win;
    int    i;
    int    j;
    input_fname param;
    int int_term_array[] = { '\n',  '\t', KEY_ESC, KEY_UP, KEY_DOWN };
    terminal_chars int_terminal;
    int    intch;
    int    state = 0;
    int    width;
    char   prompt_str[128];
    char   minimum_time_str[128];
    char   silence_threshold_str[128];
    char   silence_repeat_str[128];
    char   input_file_str[1024];
    struct prompts_t { char *prompt; char *value;};
    struct prompts_t prompts[] =
        {
          {"Input File:        ", NULL },
          {"Minimum time:      ", NULL },
          {"Silence decibels: -", NULL },
          {"Silence repeat:    ", NULL },
          { NULL,                 NULL }
        };
    
#   define AUTO_WINDOW_DISPLAY_WIDTH   56
#   define AUTO_WINDOW_FIELD_WIDTH     (AUTO_WINDOW_DISPLAY_WIDTH - 2)
#   define AUTO_WINDOW_FIELD_FMT   " %-52s"
#   define AUTO_WINDOW_LINES           8
#   define AUTO_WINDOW_COLUMNS         (AUTO_WINDOW_DISPLAY_WIDTH-1)
#   define AUTO_WINDOW_Y_POSITION      3
#   define AUTO_WINDOW_X_POSITION      (((maxx/2)-40)+2)
#   define AUTO_WINDOW_LEFT_MARGIN     1
#   define AUTO_MENU_ENTRIES_CNT (sizeof(prompts)/sizeof(struct prompts_t)-1)

    int_terminal.values = int_term_array;
    int_terminal.len    = sizeof(int_term_array)/sizeof(int);

    prompts[0].value = input_file_str;
    prompts[1].value = minimum_time_str;
    prompts[2].value = silence_threshold_str;
    prompts[3].value = silence_repeat_str;
    sigwinch_ctx.autoedit = win = 
        newwin(AUTO_WINDOW_LINES,       AUTO_WINDOW_COLUMNS, 
               AUTO_WINDOW_Y_POSITION,  AUTO_WINDOW_X_POSITION);
    if (!win) {
        return -1;
    }
    keypad(win, TRUE);

    /*
     * Initialize window
     */
    wclrtobot(win);
    wattron(win, A_REVERSE);
    for (i=0; i<maxy-6; i++) {
        mvwprintw(win, i, 0, "%*c", AUTO_WINDOW_DISPLAY_WIDTH, ' ');
    }

    /*
     * Display window with current edit menu values
     */
    i=0;
    mvwprintw(win, i++,
              (AUTO_WINDOW_DISPLAY_WIDTH/2) - 7, " mpgedit autoedit ");
    wattroff(win, A_REVERSE);

    mvwprintw(win, i++, AUTO_WINDOW_LEFT_MARGIN, 
              "%*c", AUTO_WINDOW_DISPLAY_WIDTH-3, ' ');
    strcpy(input_file_str, input_file);
    sprintf(minimum_time_str,      "%d",
            mpgedit_pcmlevel_get_minimum_time(levels));
    sprintf(silence_threshold_str, "%d",
            mpgedit_pcmlevel_get_silence_decibels(levels) * 6);
    sprintf(silence_repeat_str,    "%d",
            mpgedit_pcmlevel_get_silence_repeat(levels));
    for (j=0; prompts[j].prompt; j++) {
        width = AUTO_WINDOW_FIELD_WIDTH - strlen(prompts[j].prompt) - 3;
        sprintf(prompt_str, "%s%s",
                prompts[j].prompt, 
                (int) strlen(prompts[j].value) > width ?
                prompts[j].value + (strlen(prompts[j].value) - width) :
                prompts[j].value);
        mvwprintw(win, i++, AUTO_WINDOW_LEFT_MARGIN,
                            AUTO_WINDOW_FIELD_FMT, prompt_str);
    }
    mvwprintw(win, i++, AUTO_WINDOW_LEFT_MARGIN,
              "%*c", AUTO_WINDOW_DISPLAY_WIDTH-3, ' ');

    /*
     * Process main edit menu event loop
     */
    do {
        memset(&param, 0, sizeof(param));
        param.len = AUTO_WINDOW_FIELD_WIDTH-1;
            strcpy(param.name, prompts[state].value);
        intch = curs_read_fname(win, state+2, 2, 1, &param, 
                                prompts[state].prompt, &int_terminal);
        if (param.name[0]) {
            strcpy(prompts[state].value, param.name);
        }
        width = AUTO_WINDOW_FIELD_WIDTH - strlen(prompts[state].prompt) - 3;
        sprintf(prompt_str, "%s%s",
                prompts[state].prompt, 
                (int) strlen(prompts[state].value) > width ?
                prompts[state].value + (strlen(prompts[state].value) - width) :
                prompts[state].value);
        mvwprintw(win, state+2, AUTO_WINDOW_LEFT_MARGIN,
                  AUTO_WINDOW_FIELD_FMT, prompt_str);
        wrefresh(win);
        if (intch == '\t' || intch == KEY_DOWN) {
            state = (state + 1) % AUTO_MENU_ENTRIES_CNT;
        }
        else if (intch == KEY_UP) {
            state = (state-1 < 0) ? AUTO_MENU_ENTRIES_CNT-1 : state-1;
        }
    } while (intch != '\n' && intch != KEY_ESC);

    /*
     * Set the values from the edit menu set by the user
     */
    if (intch != KEY_ESC) {
        /* input_file is already set */
        strcpy(input_file, input_file_str);
        mpgedit_pcmlevel_set_minimum_time(levels,
                                          atoi(minimum_time_str));
        mpgedit_pcmlevel_set_silence_decibels(levels,
                                              atoi(silence_threshold_str) / 6);
        mpgedit_pcmlevel_set_silence_repeat(levels,
                                            atoi(silence_repeat_str));
        wrefresh(win);
    }
    sigwinch_ctx.autoedit = NULL;
    delwin(win);
    return intch;
}


int curs_load_autoedit(char *autoedit_file,
                       mpgedit_pcmlevel_t *levelconf,
                       edit_window *ewin,
                       edit_array *earray,
                       WINDOW *output,
                       int winx,
                       int *startline)
{
    mpeg_time *segments;
    int       segmentslen;
    int       line;
    int       jndx;
    char      stime[128];
    char      etime[128];
    int       sts;
    mpgedit_pcmlevel_t *tmp_levelconf;

    tmp_levelconf = mpgedit_pcmlevel_new(levelconf);

    sts = mpgedit_segment_find(autoedit_file, tmp_levelconf,
                               &segments, &segmentslen);
    mpgedit_pcmlevel_free(tmp_levelconf);
    if (sts) {
        return 1;
    }

    for (line=startline?*startline : 0, jndx=1; jndx<segmentslen; jndx++) {
        sprintf(stime, "%6ld:%02ld.%03ld",
                segments[jndx-1].units/60,
                segments[jndx-1].units%60,
                segments[jndx-1].usec/1000);
        sprintf(etime, "%6ld:%02ld.%03ld",
                segments[jndx].units/60,
                segments[jndx].units%60,
                segments[jndx].usec/1000);
        curs_insert_edit_line(autoedit_file, stime,
                              etime, line, ewin, earray, output, winx);
        line++;
    }
    ewin->y--;

    earray->i         = 0;
    earray->start     = 0;
    ewin->y           = 0;
    earray->el_cursor = earray->el_lines[earray->i];
    earray->intime    = earray->el_cursor->it_cursor;
    curs_edit_array_display(ewin, earray, output);

    if (startline) {
        *startline = line;
    }

    return 0;
}


void curs_clear_editor(edit_window *ewin,
                       edit_array *earray,
                       delete_line_array *undos,
                       WINDOW *output,
                       int winx)
{
    int i;

    /*
     * Clear editor
     */
    for (i=0; i<earray->max; i++) {
         curs_edit_line_delete(earray->el_lines[i], NULL);
         earray->el_lines[i] = NULL;
    }
    earray->max = 0;
    for (i=0; i<undos->i; i++) {
         curs_edit_line_delete(undos->el_lines[i], NULL);
         undos->el_lines[i] = NULL;
    }
    undos->i = 0;
    earray->el_lines[0] = curs_edit_line_init(winx);
    if (!earray->el_lines[0]) {
        fprintf(stderr, "curs_edit_line_init(eline) failed\n");
    }
    curs_set_fname(&earray->el_lines[0]->filename, "-input MP3 file-");
    earray->el_cursor = earray->el_lines[0];
    earray->intime    = earray->el_cursor->it_cursor;
    earray->save_editline = *earray->el_cursor;

    earray->dirty            = 0;
    earray->already_loaded   = 0;
    earray->i                = 0;
    earray->start            = 0;
    ewin->y                  = 0;

    curs_edit_array_display(ewin, earray, output);
}


int is_editline_modified(edit_array *earray)
{
    edit_line *cursor = earray->el_cursor;
    edit_line *saved  = &earray->save_editline;

    if (cursor->stime.min.value  != saved->stime.min.value)  return 1;
    if (cursor->stime.sec.value  != saved->stime.sec.value)  return 1;
    if (cursor->stime.msec.value != saved->stime.msec.value) return 1;

    if (cursor->etime.min.value  != saved->etime.min.value)  return 1;
    if (cursor->etime.sec.value  != saved->etime.sec.value)  return 1;
    if (cursor->etime.msec.value != saved->etime.msec.value) return 1;
    if (strcmp(cursor->filename.name, saved->filename.name)) return 1;
    return 0;
}


int curs_prompt_yes_no_response(char *prompt, edit_window *ewin, WINDOW *output)
{

    input_fname quit_response;
    int         inchar;
    int         response = 0;
    int         winx, winy;

    getmaxyx(sigwinch_ctx.root, winy, winx);
    curs_input_fname_init(&quit_response, winx-1);

    strcpy(quit_response.name, "");
    do {
        inchar = curs_read_fname(output, 2, 0, ewin->resize_x,
                                 &quit_response,
                                 prompt, NULL);
    } while (inchar != '\n' && inchar != KEY_ESC);
    response = quit_response.name[0] == 'y' || quit_response.name[0] == 'Y';
    return response;
}


int curs_play(cmdflags   *argvflags,
              editspec_t **ret_edits,
              mpgedit_pcmlevel_t *levelconf)
{
    edit_array          earray;
    edit_window         ewin;
    input_fname         out_filename;
    input_fname         abandoned_filename;
    input_fname         quit_response;
    input_time          *lasttime = NULL;
    input_time          tmp_stime;
    input_fname         save_fname;
    WINDOW              *output;
    WINDOW              *rootwin;
    int                 inchar;
    int                 quit = 0;
    int                 status;
    char                stime_fmt[32];
    char                etime_fmt[32];
    char                duration_time_fmt[32];
    editspec_t          *edits = NULL;
    int                 winy, winx;
    int                 init_winy, init_winx;
    int                 i;
    delete_line_array   undos;
    edit_line           *tmp_line;
    edit_line           *undo_line;
    edit_line           **realloc_lines;
    int                 sts;
    char                *errstr;
    int                 vol;
    char                *file;
    char                autoedit_file[1024];

    mpegfio_iocallbacks ttyio;
    editspec_t          *filenames;
    int                 filenames_cnt;
    int                 *outfile_flag;
    char                **ret_out_filename;
    int                 input_file_cursor;
    int                 D_flag;
    mpeg_time           *stime;     
    mpeg_time           *etime;     
    long                secs;
    long                usecs;
    char                **name_list;

    /* TBD: compute width based upon window width */
    char fmtbuf[1024];

    filenames        = argvflags->edits;
    filenames_cnt    = mpgedit_editspec_get_length(filenames);
    ret_out_filename = &argvflags->out_filename;
    outfile_flag     = &argvflags->O_flag;
    D_flag           = argvflags->D_flag;

    rootwin = initscr();
    if (!rootwin) {
        fprintf(stderr, "curs_play: Curses initialization initscr() failed\n");
        return 1;
    }
    noecho();
    raw();
    curs_set(_CURSOR_VISIBLE);

    /* Get size of screen for later use */
    getmaxyx(rootwin, init_winy, init_winx);
    winy = init_winy;
    winx = init_winx;

    /* Place title information on the root window; this shows through */
    mvprintw(0, INPUTFILE_XPOS, "Input file");
    mvprintw(0, winx-CURS_TIMEFIELD_WIDTH+1, "Start time    End time");
    refresh();

    stime_fmt[0] = '\0';
    etime_fmt[0] = '\0';

    status = curs_edit_window_init(&ewin, winy, winx);
    if (!status) {
        fprintf(stderr, "curs_edit_window_init failed\n");
        return 1;
    }

    curs_delete_line_array_init(&undos);
    memset(&earray, 0, sizeof(earray));

    /* Allocate buffer based upon window size */

    earray.alloc_max = filenames_cnt > winy ? filenames_cnt : winy;
    earray.alloc_max *= 2;
    earray.el_lines = (edit_line **) 
                          calloc(earray.alloc_max, sizeof(edit_line *));
    if (!earray.el_lines) {
        fprintf(stderr, "failed allocating initial edit_line array\n");
        return 1;
    }

    status = curs_input_fname_init(&out_filename, winx-1);
    if (!status) {
        fprintf(stderr, "curs_input_fname_init(out_filename) failed\n");
        return 1;
    }

    status = curs_input_fname_init(&abandoned_filename, winx-1);
    if (!status) {
        fprintf(stderr, "curs_input_fname_init(abandoned_filename) failed\n");
        return 1;
    }
    status = curs_input_fname_init(&quit_response, winx-1);
    if (!status) {
        fprintf(stderr, "curs_input_fname_init(quit_response) failed\n");
        return 1;
    }

    output = newwin(3, winx, winy-3, 0);
    if (!output) {
        fprintf(stderr, "newwin(output) failed\n");
        return 1;
    }
    scrollok(output, TRUE);
    keypad(output, TRUE);

    /*
     * I/O Context for playback function. 
     */
    memset(&ttyio, 0, sizeof(ttyio));
    ttyio.getch  = ttyio_getch;
    ttyio.printf = ttyio_printf;
    ttyio.ctx    = output;

    autoedit_file[0] = '\0';
    if (filenames_cnt) {
        for (i=0; i<filenames_cnt; i++) {
            earray.el_lines[i] = curs_edit_line_init(winx);
            if (!earray.el_lines[i]) {
                fprintf(stderr, "curs_edit_line_init(eline) failed\n");
                return 1;
            }
            earray.el_cursor = earray.el_lines[i];
            file = mpgedit_editspec_get_file(filenames, i);
            status = curs_call_indexfile(file, output);
            if (status) {
                curs_error_refresh(&ewin, &earray, output);
            }
            curs_set_fname(&earray.el_lines[i]->filename, file);

            stime = mpgedit_editspec_get_stime(filenames, i);
            mpeg_time_gettime(stime, &secs, &usecs);
            earray.el_lines[i]->stime.min.value  = secs / 60;
            earray.el_lines[i]->stime.sec.value  = secs % 60;
            earray.el_lines[i]->stime.msec.value = usecs /1000;

            etime = mpgedit_editspec_get_etime(filenames, i);
            mpeg_time_gettime(etime, &secs, &usecs);
            earray.el_lines[i]->etime.min.value  = secs / 60;
            earray.el_lines[i]->etime.sec.value  = secs % 60;
            earray.el_lines[i]->etime.msec.value = usecs /1000;

            earray.max++;
            if (D_flag) {
                strcat(autoedit_file, file);
                if ((i+1) < filenames_cnt) {
                    strcat(autoedit_file, " ");
                }
            }
        }
        earray.max--;
    }
    else {
        earray.el_lines[0] = curs_edit_line_init(winx);
        if (!earray.el_lines[0]) {
            fprintf(stderr, "curs_edit_line_init(eline) failed\n");
            return 1;
        }
        curs_set_fname(&earray.el_lines[0]->filename, "-input MP3 file-");
    }
    earray.el_cursor = earray.el_lines[0];
    earray.intime    = earray.el_cursor->it_cursor;

    if (!argvflags->D_flag && filenames_cnt == 0) {
        curs_display_help(winy, winx);
    }
    curs_edit_array_display(&ewin, &earray, output);
    earray.save_editline = *earray.el_cursor;

    wrefresh(output);
    wrefresh(ewin.win);

    curs_install_sigwinch(&ewin, &earray, rootwin, output,  &winy, &winx);

    do {
        if (D_flag) {
            inchar = 'A';
        }
        else {
            inchar = curs_read_time(&ewin,
                                    earray.el_lines[earray.i],
                                    earray.intime);
        }
        wmove(output, 0, 0);
        switch (inchar) {

          /* Auto Edit analysis */
          case 'A':
            earray.dirty |= is_editline_modified(&earray);
            if (earray.dirty) {
                i = curs_prompt_yes_no_response(
                        "Editor modified, discard anyway?: ", &ewin, output);
                if (!i) {
                    curs_edit_array_display(&ewin, &earray, output);
                    break;
                }
            }

            /*
             * Setup defaults for decoding when not specified from
             * command line arugments.
             */
            if (!argvflags->D_flag) {
                argvflags->D_flag = 1;
                argvflags->d_flag = 1;
                argvflags->d_val  = 9;
            }
            if (!D_flag) {
                i = curs_display_autoedit_menu(winy, winx, levelconf, 
                                               autoedit_file, &ewin.resize_x);
                if (i == -1 || i == KEY_ESC) {
                    curs_edit_array_display(&ewin, &earray, output);
                    break;
                }
            }

            curs_clear_editor(&ewin, &earray, &undos, output, winx);
            input_file_cursor = 0;
            name_list = expand_stringnames_2list(autoedit_file);
            for (i=0; name_list[i] && *name_list[i]; i++) {
                status = curs_call_indexfile(name_list[i], output);
                if (status) {
                    curs_error_refresh(&ewin, &earray, output);
                    break;
                }

                curs_call_decodefile(name_list[i], argvflags, output);
                curs_load_autoedit(name_list[i], levelconf,
                                   &ewin, &earray, output, winx,
                                   &input_file_cursor);
            }
            expand_stringnames_2list_free(name_list);

            D_flag                = 0;
            earray.already_loaded = 1;
            earray.intime         = earray.el_cursor->it_cursor;
            earray.save_editline  = *earray.el_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            break;

          /* Restore current edit line when ESC is pressed */
          case KEY_ESC:
            *earray.el_cursor = earray.save_editline;
            curs_edit_line_write(&ewin,
                                 curs_edit_line_format(&ewin,
                                                       earray.el_cursor,
                                                       fmtbuf));
            wmove(ewin.win, ewin.y,
                  earray.intime->if_cursor->curxpos + ewin.resize_x);
            break;

          /* Move cursor one edit line up */
          case KEY_UP:
          case 'k':
            earray.dirty |= is_editline_modified(&earray);
            earray.i--;
            if (earray.i < 0) {
                earray.i = 0;
            }

            ewin.y--;
            if (ewin.y < 0) {
                ewin.y = 0;
                earray.start--;
                if (earray.start < 0) {
                    earray.start = 0;
                }
            }
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            wmove(ewin.win, ewin.y,
                  earray.intime->if_cursor->curxpos + ewin.resize_x);
            break;
    
          /* Move cursor down */

          case KEY_DOWN:
          case 'j':
            earray.dirty |= is_editline_modified(&earray);
            if (earray.i < earray.max) {
                earray.i++;
                if ((ewin.y+1) < MAXY(winy)) {
                    ewin.y++;
                }
                else {
                    earray.start++;
                }
                earray.el_cursor = earray.el_lines[earray.i];
            }
            earray.intime = earray.el_cursor->it_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            wmove(ewin.win, ewin.y,
                  earray.intime->if_cursor->curxpos + ewin.resize_x);
            break;

          /* Join current line end time with next line end time */

          case 'J':
            if ((earray.i >= earray.max) ||
                strcmp(earray.el_cursor->filename.name,
                       earray.el_lines[earray.i+1]->filename.name))
            {
                /*
                 * Can't join different files, or when last file in
                 * the editor.
                 */
                break;
            }

            /*
             * Save a copy of the original unjoined line in the undo buffer,
             * join the next line with the current line, save the
             * next line in the undo buffer, then delete the next line.
             */
            tmp_line = curs_edit_line_init(init_winx);
            if (tmp_line) {
                curs_edit_line_set(tmp_line,
                                   earray.el_cursor->filename.name,
                                   &earray.el_cursor->stime,
                                   &earray.el_cursor->etime);
                curs_edit_line_delete(tmp_line, &undos);
            }
            curs_edit_line_set(earray.el_cursor, NULL,
                               NULL,
                               &earray.el_lines[earray.i+1]->etime);
            curs_edit_line_delete(earray.el_lines[earray.i+1], &undos);
            for (i=earray.i+1; i<earray.max; i++) {
                earray.el_lines[i] = earray.el_lines[i+1];
            }
            earray.max--;
            curs_edit_array_display(&ewin, &earray, output);
            break;
  
          /* Paste previously deleted line, above cursor line */

          case 'P':
            undo_line = curs_edit_line_undelete(&undos);
            if (!undo_line) {
                break;
            }

            /* Move all entries starting at cursor down one line */

            for (i=earray.max; i>=earray.i; i--) {
                earray.el_lines[i+1] = earray.el_lines[i];
            }
            earray.el_lines[earray.i] = undo_line;
            earray.max++;
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            break;

          /* Paste previously deleted line, below cursor line */

          case 'p':
            undo_line = curs_edit_line_undelete(&undos);
            if (!undo_line) {
                break;
            }

            /* Move all entries starting at cursor down one line */

            for (i=earray.max; i>=earray.i; i--) {
                earray.el_lines[i+1] = earray.el_lines[i];
            }
            earray.i++;
            if ((ewin.y+1) < MAXY(winy)) {
                ewin.y++;
            }
            else {
                earray.start++;
            }
            earray.el_lines[earray.i] = undo_line;
            earray.max++;
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            break;

          /* Add line below current cursor position */

          case 'o':
            earray.dirty = 1;
            /*
             * Allocate more space for edit array
             */
            if (earray.max + 1 >= earray.alloc_max) {
                realloc_lines = (edit_line **)
                    realloc(earray.el_lines,
                            sizeof(edit_line *) * earray.alloc_max * 2);
                if (!realloc_lines) {

                    /* Not enough memory for operation */

                    mvwprintw(output, 2, 0,
                              "Failed allocating more memory for edit buffer");
                    break;
                }
                else {
                    earray.el_lines = realloc_lines;
                    earray.alloc_max *= 2;
                }
            }

            /*
             * Shift viewport in edit array down one line when the
             * cursor is at the bottom of the screen.
             */
            if ((ewin.y+1) >= MAXY(winy)) {
                earray.start++;
            }
            else {
                ewin.y++;
            }


            /* Move all entries starting at cursor down one line */

            for (i=earray.max; i>=earray.i; i--) {
                earray.el_lines[i+1] = earray.el_lines[i];
            }
            earray.i++;

            earray.el_lines[earray.i] = curs_edit_line_init(init_winx);
            if (!earray.el_lines[earray.i]) {
                fprintf(stderr, "curs_edit_line_init(eline) failed\n");
                return 1;
            }
            earray.max++;
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            tmp_line         = earray.el_lines[earray.i-1];

            if (earray.i > 0) {
                /*
                 * Initialize new line filename to previous line's name
                 */
                curs_edit_line_set(earray.el_cursor,
                                   tmp_line->filename.name,
                                   &tmp_line->etime,
                                   NULL);
            }

            curs_edit_line_format(&ewin, earray.el_cursor, fmtbuf);
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            wmove(ewin.win, ewin.y,
                  earray.el_lines[earray.i]->stime.min.curxpos + ewin.resize_x);

            break;

          /* Add line above current cursor position */

          case 'O':
            earray.dirty = 1;
            /*
             * Allocate more space for edit array
             */
            if (earray.max + 1 >= earray.alloc_max) {
                realloc_lines = (edit_line **)
                    realloc(earray.el_lines,
                            sizeof(edit_line *) * earray.alloc_max * 2);
                if (!realloc_lines) {

                    /* Not enough memory for operation */

                    mvwprintw(output, 2, 0,
                              "Failed allocating more memory for edit buffer");
                    break;
                }
                else {
                    earray.el_lines = realloc_lines;
                    earray.alloc_max *= 2;
                }
            }

            /* Move all entries starting at cursor down one line */

            for (i=earray.max; i>=earray.i; i--) {
                earray.el_lines[i+1] = earray.el_lines[i];
            }

            earray.el_lines[earray.i] = curs_edit_line_init(init_winx);
            if (!earray.el_lines[earray.i]) {
                fprintf(stderr, "curs_edit_line_init(eline) failed\n");
                return 1;
            }
            earray.max++;
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            tmp_line         = earray.el_lines[earray.i-1];

            if (earray.i > 0) {
                /*
                 * Initialize new line filename to previous line's name
                 */
                curs_edit_line_set(earray.el_cursor,
                                   tmp_line->filename.name,
                                   NULL,
                                   &tmp_line->etime);
            }
            else {
                curs_set_fname(&earray.el_lines[0]->filename,
                               "-input MP3 file-");
            }
            curs_edit_line_format(&ewin, earray.el_cursor, fmtbuf);
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            wmove(ewin.win, ewin.y,
                  earray.el_lines[earray.i]->stime.min.curxpos + ewin.resize_x);
            break;
    
          case 'C':
            earray.dirty |= is_editline_modified(&earray);
            if (earray.dirty) {
                i = curs_prompt_yes_no_response(
                        "Editor modified, discard anyway?: ",
                        &ewin, output);
                if (!i) {
                    curs_edit_array_display(&ewin, &earray, output);
                    break;
                }
            }
            curs_clear_editor(&ewin, &earray, &undos, output, winx);
            break;

          case 'D':
            /*
             * Delete the edit line at the cursor position.
             */
            if (earray.max > 0) {
                earray.dirty = 1;
                if (earray.el_cursor == earray.el_lines[earray.max]) {
                    /*
                     * Cursor is positioned at the bottom of the list.
                     * Erase the line from the screen, then decrement the
                     * list size by one.
                     */
                    curs_edit_line_delete(earray.el_lines[earray.i], &undos);
                    earray.i--;
                    if (ewin.y > 0) {
                        ewin.y--;
                    }
                    else if (earray.start > 0) {
                        earray.start--;
                    }
                    earray.max--;
                    earray.el_cursor = earray.el_lines[earray.i];
                    earray.intime    = earray.el_cursor->it_cursor;
                    curs_edit_array_display(&ewin, &earray, output);
                }
                else {
                    curs_edit_line_delete(earray.el_lines[earray.i], &undos);

                    /*
                     * Must copy everything below the current cursor position
                     * up in the array.  This is where a linked list would
                     * be much nicer.
                     */
                    for (i=earray.i; i<earray.max; i++) {
                        earray.el_lines[i] = earray.el_lines[i+1];
                    }

                    earray.max--;
                    earray.el_cursor = earray.el_lines[earray.i];
                    earray.intime    = earray.el_cursor->it_cursor;
                    curs_edit_array_display(&ewin, &earray, output);
                }
                earray.save_editline = *earray.el_cursor;
            }
            break;

          case 'n':
            inchar = curs_get_out_filename(&out_filename,
                                           "Output file: ",
                                           "Output file exists, overwrite? ",
                                           output, ewin.resize_x, 1);
            if (inchar == KEY_ESC) {
                out_filename.name[0] = '\0';
            }
            curs_edit_array_display(&ewin, &earray, output);
            break;
    
          case 'f':
            save_fname = earray.el_cursor->filename;
            inchar = curs_read_fname(ewin.win, ewin.y, ewin.x, ewin.resize_x,
                                     &earray.el_cursor->filename, NULL, NULL);

            if (inchar == KEY_ESC) {
                earray.el_cursor->filename = save_fname;
                curs_edit_array_display(&ewin, &earray, output);
            }
            else {
                name_list = expand_stringnames_2list_sep(
                                earray.el_cursor->filename.name, 0, 0);
                if (name_list) {
                    curs_set_fname(&earray.el_cursor->filename, name_list[0]);
                    expand_stringnames_2list_free(name_list);
                }
                status = curs_call_indexfile(earray.el_cursor->filename.name,
                                             output);
                if (!status) {
                    earray.el_lines[earray.i]->stime.min.value  = 0;
                    earray.el_lines[earray.i]->stime.sec.value  = 0;
                    earray.el_lines[earray.i]->stime.msec.value = 0;
                }
                curs_edit_array_display(&ewin, &earray, output);
            }
            wmove(ewin.win, ewin.y,
                  earray.intime->if_cursor->curxpos + ewin.resize_x);
            break;
    
          /* Toggle between stime/etime, moving cursor to right */
          case '\t':
          case 'l':
          case KEY_RIGHT:
          case ' ':
            if (earray.intime == &earray.el_cursor->stime) {
                earray.el_cursor->etime.if_cursor =
                    &earray.el_cursor->etime.min;
                earray.el_cursor->it_cursor       =
                    &earray.el_cursor->etime;
            }
            else {
                earray.el_cursor->stime.if_cursor =
                    &earray.el_cursor->stime.min;
                earray.el_cursor->it_cursor       =
                    &earray.el_cursor->stime;
            }
            earray.intime = earray.el_cursor->it_cursor;
            wmove(ewin.win, ewin.y,
                  earray.intime->if_cursor->curxpos + ewin.resize_x);
            break;

          /* Toggle between stime/etime, moving cursor to left */
          case 'h':
          case KEY_LEFT:
            if (earray.intime == &earray.el_cursor->stime) {
                earray.el_cursor->etime.if_cursor =
                    &earray.el_cursor->etime.msec;
                earray.el_cursor->it_cursor       =
                    &earray.el_cursor->etime;
            }
            else {
                earray.el_cursor->stime.if_cursor =
                    &earray.el_cursor->stime.msec;
                earray.el_cursor->it_cursor       =
                    &earray.el_cursor->stime;
            }
            earray.intime = earray.el_cursor->it_cursor;
            wmove(ewin.win, ewin.y,
                  earray.intime->if_cursor->curxpos + ewin.resize_x);
            break;
    
          /* Sound playback beginning at the current time field */

          case 's':
          case 'S':
            if (earray.el_cursor->filename.name &&
                earray.el_cursor->filename.name[0]) 
            {
                if (lasttime) {
                    earray.intime = lasttime;
                    lasttime = NULL;
                }
                wmove(output, 2, 0);
                nodelay(rootwin, TRUE);

                memset(&tmp_stime, 0, sizeof(tmp_stime));
                if (inchar == 's') {
                    strcpy(duration_time_fmt, "1000000:00.0");
                    if (earray.intime == &earray.el_cursor->stime) {
                        tmp_stime = earray.el_cursor->stime;
                    }
                    else {
                        tmp_stime = earray.el_cursor->etime;
                    }
                    curs_time2string(&tmp_stime, stime_fmt);
                }
                else {
                    strcpy(duration_time_fmt, "0:05.0");
                    if (earray.intime == &earray.el_cursor->stime) {
                        tmp_stime = earray.el_cursor->stime;
                    }
                    else {
                        tmp_stime = earray.el_cursor->etime;
                    }
                    tmp_stime.sec.value -= 5;
                    if (tmp_stime.sec.value < 0) {
                        tmp_stime.sec.value += 60;
                        tmp_stime.min.value -= 1;
                    }
                    if (tmp_stime.min.value < 0) {
                        tmp_stime.min.value = tmp_stime.sec.value = 0;
                    }
                    curs_time2string(&tmp_stime, stime_fmt);
                }
                
                status = 
                    curs_interactive_play_file(earray.el_cursor->filename.name,
                                               stime_fmt,
                                               duration_time_fmt,
                                               &ttyio);

                nodelay(rootwin, FALSE);
                if (status) {
                    wmove(output, 0, 0);
                    wclrtobot(output);
                    if (status == 1 || status == 4) {
                        curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                  "Can't open index file for '%s'",
                                  earray.el_cursor->filename.name);
                 
                    }
                    else if (status == 2) {
                        curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                         "Can't open play file '%s'",
                                  earray.el_cursor->filename.name);
                    }
                    else if (status == 3) {
                        curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                  "Can't open external MP3 player");
                    }
                    else if (status == 5) {
                        curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                         "Invalid seek time");
                    }
                    else {
                        curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                         "Error playing file '%s'",
                                         earray.el_cursor->filename.name);
                    }
                }
            }
            else {
                wmove(output, 0, 0);
                wclrtobot(output);
                mvwprintw(output, 2, 0,  "No input file specified");
            }
    
            break;

          /* Move cursor to top of edit array */

          case 'H':
            earray.dirty |= is_editline_modified(&earray);
            earray.i         = 0;
            earray.start     = 0;
            ewin.y           = 0;
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            break;

          /* Move cursor to bottom of edit array */

          case 'G':
            earray.dirty |= is_editline_modified(&earray);
            earray.i      = earray.max;
            earray.start  = earray.max - MAXY(winy) + 1;
            if (earray.start < 0) {
                earray.start = 0;
            }
            ewin.y        = MAXY(winy) - 1;
            if (ewin.y > earray.max) {
                ewin.y = earray.max;
            }
            earray.el_cursor = earray.el_lines[earray.i];
            earray.intime    = earray.el_cursor->it_cursor;
            curs_edit_array_display(&ewin, &earray, output);
            earray.save_editline = *earray.el_cursor;
            break;

          /* Load abandoned edits file */
          case 'L':
            if (earray.already_loaded) {
                wmove(output, 2, 0);
                wclrtoeol(output);
                curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                 "Edits have already been loaded");

                curs_edit_array_display(&ewin, &earray, output);
                break;
            }

            curs_get_default_abandoned_filename(abandoned_filename.name);
            if (!abandoned_filename.name[0]) {
                strcpy(abandoned_filename.name, ".mp3edit_abandoned");
            }
            inchar = curs_get_abandoned_filename(&abandoned_filename,
                                                 ewin.resize_x, output);
            if (inchar != KEY_ESC && abandoned_filename.name[0]) {
                sts = curs_load_abandoned_edits(abandoned_filename.name,
                                                &ewin, &earray, output, winx);
                if (sts == 0) {
                    earray.already_loaded = 1;
                    curs_put_default_abandoned_filename(
                        abandoned_filename.name);
                    earray.intime = earray.el_cursor->it_cursor;
                    earray.save_editline = *earray.el_cursor;
                }
                else {
                    curs_warn_printw(output, 2, 0, PRESS_RETURN,
                        "Failed loading abandoned edits file");
                    curs_edit_array_display(&ewin, &earray, output);
                }
            }
            else {
                curs_edit_array_display(&ewin, &earray, output);
            }
            break;

          case 'Q':
            i = curs_prompt_yes_no_response("Quit current edits?: ",
                                            &ewin, output);
            if (i) {
                abandoned_filename.name[0] = '\0';
                quit = -1;
            }   
            else {
                curs_edit_array_display(&ewin, &earray, output);
            }
            break;

          case 'q':
            curs_get_default_abandoned_filename(abandoned_filename.name);
            if (!abandoned_filename.name[0]) {
                strcpy(abandoned_filename.name, ".mp3edit_abandoned");
            }
            do {
                inchar = curs_get_out_filename(
                             &abandoned_filename,
                             "Abandoned file: ",
                             "Abandoned edits file exists, overwrite? ",
                             output, ewin.resize_x, 0);
            } while (!abandoned_filename.name[0] && inchar != KEY_ESC);
            if (inchar != KEY_ESC && abandoned_filename.name[0]) {
                quit = -1;
                curs_put_default_abandoned_filename(abandoned_filename.name);
            }
            else {
                curs_edit_array_display(&ewin, &earray, output);
            }
            break;

          case 'e':
          case 'E':
            /*
             * 'E' performs same action as 'e', except will write all edits
             * to derived file names, instead of requiring the specification
             * of an output filename.
             */
            if (inchar == 'E') {
                *outfile_flag = 1;
            }

            edits = curs_build_editspec(&earray, &status);
            if (status) {
                wmove(output, 0, 0);
                wclrtobot(output);
                if (status == 3) {
                    curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                     "Missing filename in edit specification");
                }
                else {
                    curs_warn_printw(output, 2, 0, PRESS_RETURN,
                                     "Error with edit time specified");
                }
                curs_edit_array_display(&ewin, &earray, output);
            }
            else if (!*outfile_flag &&(!out_filename.name ||
                     !*out_filename.name))
            {
                wmove(output, 0, 0);
                wclrtobot(output);
                curs_warn_printw(output, 2, 0,
                                 PRESS_RETURN, "Output filename required");

                curs_edit_array_display(&ewin, &earray, output);
            }
            else {
                errstr = curs_validate_edit_times(edits);
                if (errstr) {
                    wmove(output, 0, 0);
                    wclrtobot(output);
                    curs_warn_printw(output, 2, 0, PRESS_RETURN, "%s", errstr);

                    curs_edit_array_display(&ewin, &earray, output);
                    break;
                }

                curs_get_default_abandoned_filename(abandoned_filename.name);
                if (!abandoned_filename.name[0]) {
                    strcpy(abandoned_filename.name, ".mp3edit_abandoned");
                }
                do {
                    inchar =
                        curs_get_out_filename(
                            &abandoned_filename,
                            "Abandoned file: ", 
                            "Abandoned edits file exists, overwrite? ",
                            output, ewin.resize_x, 0);
                } while (!abandoned_filename.name[0] && inchar != KEY_ESC);
                if (inchar == KEY_ESC) {
                    curs_edit_array_display(&ewin, &earray, output);
                }
                else if (abandoned_filename.name[0]) {
                    wmove(output, 0, 0);
                    wclrtobot(output);
                    curs_put_default_abandoned_filename(
                        abandoned_filename.name);
                    quit = 1;
                }
            }
            if (quit == 0) {
                *outfile_flag = 0;
            }
            break;

          case 'v':
            vol = curs_get_volume() - 2;
            vol = (vol < 0) ? 0 : vol;
            curs_set_volume(vol);
            mvwprintw(output, 2, VOLUME_XPOS, "v:%3d", vol);
            break;
          
          case 'V':
            vol = curs_get_volume() + 2;
            vol = (vol > 100) ? 100 : vol;
            curs_set_volume(vol);
            mvwprintw(output, 2, VOLUME_XPOS, "v:%3d", vol);
            break;
          
          case KEY_CTRL_L:
            curs_edit_array_display(&ewin, &earray, output);
            break;

          case '?':
            curs_display_help(winy, winx);
            curs_edit_array_display(&ewin, &earray, output);
            break;
           
          default:
            mvwprintw(output, 0, 0,  "Read character %c %d %x",
                      inchar, inchar, inchar);
            break;
        }
        wrefresh(output);
        wrefresh(ewin.win);
    } while (!quit);

    /* Final refresh clears screen at end */
    clrtobot();
    refresh();

    mvprintw(2, 0, "");
    refresh();

    if (ret_edits && edits && mpgedit_editspec_get_length(edits) > 0) {
        *ret_edits = edits;
    }
    if (out_filename.name && *out_filename.name) {
        if (ret_out_filename) {
            *ret_out_filename = strdup(out_filename.name);
        }
    }
    endwin();
    if (abandoned_filename.name[0]) {
        fprintf(stderr, "Saving abandoned edits\n");
        curs_save_abandoned_edits(&earray, abandoned_filename.name);
    }
    return quit < 0;
}
