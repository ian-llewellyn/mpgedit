/*
 * xmpgedit main UI application
 *
 * Copyright (C) 2001-2008 Adam Bernstein. 
 * All Rights Reserved.
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

#include <gtk/gtk.h>
#include <gtk/gtkenums.h>
#include <gtk/gtkrange.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include "playif.h"
#include "editif.h"
#include "editor.h"
#include "volume.h"
#include "pcmview.h"
#include "../segment.h"
#include "../version.h"
#include "../pcmlevel.h"
#include "../wav16.h"
#include "../mpegindx.h"
#include "playctrl.h"
#include "player.h"
#include "ledtime.h"

#if 0
#define _DEBUG 1
#endif

#if 0  /* Not sure if I like this control or not */
#define LEDPCM_DISPLAY 1
#endif

#define PLAYTO_BACKUP_SEC 5
#define PLAY_IDLE_MSEC 5  /* Probably should be configurable */

#include "../portability.h"

#define TIP_PLAYTO       "Play previous 5 seconds"
#define TIP_PAUSE        "Pause playback"
#define TIP_STOP         "Stop playback"
#define TIP_PLAY         "Play from current time"
#define TIP_RECORD       "Record current time"
#define TIP_RECORD_START "Record start time"
#define TIP_RECORD_STOP  "Record end time"
#define TIP_PCMVIEW_LEFT "Scroll left"
#define TIP_PCMVIEW_RIGHT "Scroll right"
#define TIP_PCMVIEW_SAVE "Save current edit"

typedef enum {
    EDIT_ACTION_COPY = 1,
    EDIT_ACTION_DELETE,
    EDIT_ACTION_PASTE,
    EDIT_ACTION_INSERT,
    EDIT_ACTION_TIME,
    EDIT_ACTION_STIME,
    EDIT_ACTION_DUP,
    EDIT_ACTION_CHAIN,

    MAINMENU_ACTION_ABOUT,
    MAINMENU_ACTION_OPTIONS,
    MAINMENU_ACTION_LOAD_ABANDONED,
    MAINMENU_ACTION_OUTFILE,
    MAINMENU_ACTION_SAVE_ABANDON,
    MAINMENU_ACTION_EDIT,
    MAINMENU_ACTION_DECODE,
    MAINMENU_ACTION_STATS,
    MAINMENU_ACTION_OFFSET,
    MAINMENU_ACTION_CLEAR,
    MAINMENU_ACTION_OPEN,

    OPTIONMENU_ACTION_STOP,
    OPTIONMENU_ACTION_PAUSE,
    OPTIONMENU_ACTION_VOLUME,
    OPTIONMENU_ACTION_OFFSET,
} menu_action_e;


#define OUTPUT_FILE_APPEND_DEFAULT 0
#define OUTPUT_FILE_APPEND_JOIN    1
#define OUTPUT_FILE_APPEND_SPLIT   2

typedef enum {
    EDIT_IDLE_STATE_INIT,
    EDIT_IDLE_STATE_LOOP,
    EDIT_IDLE_STATE_EDIT,
    EDIT_IDLE_STATE_DONE,
    EDIT_IDLE_STATE_ERROR,
} edit_idle_state_e;


/* adam/TBD: only one index idleproc for now */
#define MAX_IDLE_INDEX_PROC 1
#define MAX_IDLE_DECODE_PROC 1
#define CONFIG_FILE_NAME ".xmpgeditrc"
#define GTK_RC_FILE      "/.xmpgedit-gtkrc"

#include "credits.h"  /* defines XMPGEDIT_CREDITS_COPYRIGHT */


typedef struct _outfile_radio_ctx {
    int button;
    int *radio_selection;
    global_vars_ctx *gvctx;
} outfile_radio_ctx;


typedef struct _record_button_ctx_t {
    int start; /* Determine start/stop button identity */
    global_vars_ctx *gvctx;
} record_button_ctx_t;


typedef struct _edit_menu_ctx {
    global_vars_ctx *gvctx;
    menu_action_e   action;
    char            *actionstr;
} edit_menu_ctx;


typedef struct _idle_indexfile_ctx {
    GtkWidget        *widget;
    GdkEvent         *event;
    gpointer         *data;
    void             *playctx;
    edit_row_ctx     *edit_row;
    long             tlast;
    long             fsize;
    char             *file_name;
    void             (*callback)(GtkWidget *, GdkEvent *, gpointer);
    int              stereo;
    int              offset;
#if 1 /* adam/TBD: PCM data context */
    int              pcmmax_last;
    int              pcmavg1;
    int              pcmavg1cnt;
    int              pcmavg2;
    int              pcmavg2cnt;
    int              pcmmin;
    int              pcmmax;
    char             *pcmlevelname;
    mpgedit_pcmfile_t *pcmh;
#endif
} idle_indexfile_ctx;

typedef struct _index_file_list {
    idle_indexfile_ctx      *ifctx;
    struct _index_file_list *next;
} index_file_list;


typedef struct _mpgedit_workqueue_t {
    index_file_list *ifl_head;
    index_file_list *ifl_tail;
} mpgedit_workqueue_t;


typedef struct _indexfile_idleproc_ctx {
    global_vars_ctx    *gvctx;
    idle_indexfile_ctx *ifctx;

    /* Progress bar controls */
    GtkWidget        *progress;
    GtkWidget        *dialog;
    GtkWidget        *label;
    int              quit;
    int              eof;
} indexfile_idleproc_ctx;


/* 
 * Flow of control: cb_edit() -> cb_edit_idle_editfile()
 */
typedef struct _idle_editfile_ctx {
    global_vars_ctx *gvctx;
    void            *edfiles_ctx;
    int             edfiles_ctx_editing;
    GtkWidget       *dialog;
    GtkWidget       *progress;
    int             total_sec;
    int             current_sec;
    int             quit;
    int             edit_flags;
    edit_idle_state_e state;
    int             edit_indx;
    int             out_fileindx;
    char            *out_filename;
    editspec_t      *edarray;
    int             edarray_len;
    editspec_t      *built_edarray;
    mpeg_file_stats edit_stats;
} idle_editfile_ctx;


typedef struct _cb_edit_ctx
{
    int        index;
    int        len;
    global_vars_ctx *gvctx;
    editspec_t *edarray;
} cb_edit_ctx;


typedef struct _ledtime_cbctx_t {
    global_vars_ctx *gvctx;
    mpgedit_ledtime_t *widget;
} ledtime_cbctx_t;


typedef struct _xmpgedit_cmd_args_t {
    editspec_t *editspec;
    char       *offset;
    long       offset_sec;
    long       offset_msec;
} xmpgedit_cmd_args_t;


void ___file_sel_ok(GtkWidget *widget, gpointer data,
                    long init_sec, long init_msec);

void  cb_row_clicked(GtkWidget *widget,
                     int row, int column,
                     GdkEventButton *event,
                     gpointer data);
void cb_edit_3(GtkWidget *widget, GdkEvent *dummy, gpointer data);
void cb_stop_button(GtkWidget *widget, gpointer data);
void cb_record_button(GtkWidget *widget, gpointer data);
void ___set_editor_defaults(global_vars_ctx *gvctx);
void cb_open_file(GtkWidget *widget, gpointer data);
void cb_open_file_from_menu(gpointer data);
gint cb_quit(GtkWidget *widget, gpointer data);
void mainmenu_item_response(edit_menu_ctx *emctx);

void cb_edit_1(GtkWidget *widget, gpointer data);
void ___load_editor_by_files(global_vars_ctx *gvctx, char **files, int len);

GtkWidget *___make_indexfile_progress_dialog(indexfile_idleproc_ctx *ctx,
                                             GtkWidget *parent,
                                             char *label,
                                             char *file_name);
void cb_row_clicked_1(GtkWidget *widget, GdkEvent *event, gpointer data);
outfile_dialog_ctx *___make_dialog_outputfile(global_vars_ctx *gvctx,
                                              char *name_label,
                                              int  show_radio,
                                              char **output_name,
                                              int  *radio_action);
void cb_dialog_outputfile(GtkWidget *dialog, gint response, gpointer data);
void cb_load(GtkWidget *widget, gint response, gpointer data);
void cb_edit_2(GtkWidget *widget, gint response,  gpointer data);
idle_indexfile_ctx *___index_file_list_get(mpgedit_workqueue_t *q);
int ___index_file_list_put(mpgedit_workqueue_t *q, idle_indexfile_ctx *ifctx);
void cb_edit_1a(GtkWidget *widget, gpointer data);
void ___queue_decode_file(global_vars_ctx *gvctx, char *file);

void cb_decode(global_vars_ctx *gvctx);
void cb_offset(global_vars_ctx *gvctx);
outfile_dialog_ctx *___make_dialog_outputfile_title(global_vars_ctx *gvctx,
                                              char *title,
                                              char *name_label,
                                              int  show_radio,
                                              char **output_name,
                                              int *radio_action);
void _adjust_ledtrt(global_vars_ctx *gvctx);

#ifdef _DEBUG
void find_fd_leaks(char *label)
{
    char dummy_sb[1024];
    int i;

    for (i=0; i<2048; i++) {
        if (fstat(i, (void *) dummy_sb) == -1) {
            printf("find_fd_leaks: max_fd = %d %s\n", i, label);
            return;
        }
    }
}
#endif

/*
 * Remove leading and trailing white space from a string.
 * Used to clean up strings from GUI text input fields.
 */
char *strtrim(const char *str)
{
    const char *cp;
    char *rcp;

    cp = str;
    while (*cp && isspace(*cp)) {
        cp++;
    }
    str = cp;
    cp = cp + strlen(cp) - 1;
    while (cp > str && isspace(*cp)) {
        cp--;
    }
    cp++;
    
    rcp = malloc(cp - str + 1);
    if (rcp) {
        strncpy(rcp, str, cp - str);
        rcp[cp - str] = '\0';
    }
    
    return rcp;
}


int get_config_value(char *str, char *name, int *val)
{
    int len;
    char *cp = str;

    len = strlen(name);
    if (strncmp(cp, name, len)) {
        return 0;
    }

    cp += len;
    while (isspace(*cp)) cp++;
    if (*cp == ':') cp++;
    while (isspace(*cp)) cp++;
    
    if (!strcmp(cp, "TRUE")) {
        *val = 1;
    }
    else if (!strcmp(cp, "FALSE")) {
        *val = 0;
    }
    else {
        *val = atoi(cp);
    }
    return 1;
            
}


int get_config_value_string(char *str, char *name, char **val)
{
    int len;
    char *cp = str;

    len = strlen(name);
    if (strncmp(cp, name, len)) {
        return 0;
    }

    cp += len;
    while (isspace(*cp)) cp++;
    if (*cp == ':') cp++;
    while (isspace(*cp)) cp++;
    
    *val = strdup(cp);
    if (!*val) {
        return 0;
    }
    return 1;
            
}


void write_config_file(char *file, global_vars_ctx *gvctx)
{
    FILE *fp    = NULL;

    fp = fopen(file, "w+");
    if (!fp) {
        goto clean_exit;
    }

    fprintf(fp, "pause_button: %s\n", gvctx->pause_visable ? "TRUE" : "FALSE");
    fprintf(fp, "stop_button:  %s\n", gvctx->stop_visable ?  "TRUE" : "FALSE");
    fprintf(fp, "volume_control: %s\n", 
            gvctx->volume_visable ?  "TRUE" : "FALSE");
    fprintf(fp, "offset_time: %s\n", gvctx->offset_visable ?  "TRUE" : "FALSE");
    if (gvctx->filesel_path) {
        fprintf(fp, "filesel_pathlast: %s\n", gvctx->filesel_path);
    }
    if (gvctx->filesel_pattern) {
        fprintf(fp, "filesel_patternlast: %s\n", gvctx->filesel_pattern);
    }

clean_exit:
    if (fp) {
        fclose(fp);
    }
}


void read_config_file(char *file, global_vars_ctx *gvctx)
{
    FILE *fp    = NULL;
    char *buf   = NULL;
    int  buflen = 1024;
    char *cp;
    char *ptr;
    char *sval;
    int  val;

    fp = fopen(file, "r");
    if (!fp) {
        goto clean_exit;
    }
    buf = malloc(buflen);
    if (!buf) {
        goto clean_exit;
    }

    do {
        cp = fgets(buf, buflen-1, fp);
        if (cp) {
            buf[strlen(buf)-1] = '\0';
            if (*cp == '#') {
                continue;
            }
            else if (get_config_value(cp, "pause_button:", &val)) {
                gvctx->pause_visable = val;
            }
            else if (get_config_value(cp, "stop_button:", &val)) {
                gvctx->stop_visable = val;
            }
            else if (get_config_value(cp, "volume_control:", &val)) {
                gvctx->volume_visable = val;
            }
            else if (get_config_value(cp, "offset_time:", &val)) {
                gvctx->offset_visable = val;
            }
            else if (get_config_value_string(cp, "filesel_pathlast:", &sval)) {
                gvctx->filesel_path = sval;
                /*
                 * Fixup path separators that are reversed for current OS
                 */
                ptr = gvctx->filesel_path;
                if (ptr) {
                    while (*ptr) {
                        if (*ptr == _DIR_SEPARATOR_CHAR_NOT) {
                            *ptr = _DIR_SEPARATOR_CHAR;
                        }
                        ptr++;
                    }
                }
            }
            else if (get_config_value_string(cp,
                     "filesel_patternlast:", &sval))
            {
                gvctx->filesel_pattern = sval;
            }
        }
    } while (!feof(fp));

clean_exit:
    if (fp) {
        fclose(fp);
    }
    if (buf) {
        free(buf);
    }
    
}


void ___add_offset_time(long min, long sec, long msec,
                        long offset_sec, long offset_msec,
                        int *rsec, int *rmsec)
{
    long tmp_sec;
    long tmp_msec;

    if (offset_sec || offset_msec) {
        tmp_sec   = min*60 + sec + offset_sec;
        tmp_msec  = msec + offset_msec;
        tmp_sec  += tmp_msec/1000;
        tmp_msec %= 1000;
        *rsec     = (int) tmp_sec;
        *rmsec    = (int) tmp_msec;
    }
    else {
        *rsec  = (int) (min * 60 + sec);
        *rmsec = (int) msec;
    }
}


static void ___pcmview_draw_cursor_sec(global_vars_ctx *gvctx,
                                       long sec,
                                       long msec,
                                       int cursor_percent)
{
    int xpos;
    mpgedit_pcmview_ctx *pcmctx = gvctx->pcmviewctx;
    
    xpos = gvctx->pcmviewctx->sec_to_xpos(gvctx->pcmviewctx, sec, msec);
    pcmctx->draw_cursor(gvctx->pcmviewctx, xpos, cursor_percent);
}


/* 
 * Convenience routine to set the open playback device context onto the
 * volume context, query the current volume from the GUI volume control
 * and then set the mixer volume to those levels.
 */
static void ___init_volume(volume_control_ctx *volumectx, void *playctx)
{
    int lvol;
    int rvol;

    /* Set playback device handle onto volume context*/
    volume_control_init_mixctx(volumectx, playctx);
    if (!playctx) {
        return;
    }

    /* Get the current GUI volume control levels */
    volume_control_get_values(volumectx, &lvol, &rvol);

    /* Initialize the mixer levels */
    mpgedit_play_volume_init(playctx, lvol, rvol);
}


static void ___edit_row_set_focus(global_vars_ctx *gvctx,
                                  int pcmseek, int row_select)
{
    edit_row_ctx *er = gvctx->editctx.cursor;
    long         sec;
    mpgedit_pcmview_ctx *pcmctx = gvctx->pcmviewctx;

#ifdef _DEBUG
    printf("___edit_row_set_focus: called\n"
           "Row:Col %d:%d selected: %s %d %d %d - %d %d %d\n", 
           er->row_num, er->col_num, er->file_name, 
           er->s_min, er->s_sec, er->s_msec,
           er->e_min, er->e_sec, er->e_msec);
#endif
    gvctx->idlectx->play_file  = strdup(er->file_name);

    /*
     * Set entry file name
     */
    gtk_tooltips_enable(gvctx->tooltips_entry);
    gtk_tooltips_set_tip(gvctx->tooltips_entry, GTK_WIDGET(gvctx->entry),
                         er->file_name, NULL);
    gtk_entry_set_text(GTK_ENTRY(gvctx->entry), er->file_name);
    gtk_editable_set_position(
        GTK_EDITABLE(gvctx->entry), strlen(er->file_name));

    if (er->col_num != 2) {
        sec = er->s_min*60 + er->s_sec;
        ___file_sel_ok(NULL, (gpointer) gvctx->idlectx,
                       sec, er->s_msec);
        if (pcmseek) {
            pcmctx->seek_sec(pcmctx, (sec >= 10) ? sec-10 : 0);
            ___pcmview_draw_cursor_sec(gvctx, sec, er->s_msec, 100);
        }
    }
    else {
        sec = er->e_min*60 + er->e_sec;
        ___file_sel_ok(NULL, (gpointer) gvctx->idlectx,
                       sec, er->e_msec);
        if (pcmseek) {
            pcmctx->seek_sec(pcmctx, (sec >= 10) ? sec-10 : 0);
            ___pcmview_draw_cursor_sec(gvctx, sec, er->e_msec, 100);
        }
    }

    if (GTK_WIDGET_VISIBLE(gvctx->pcmviewctx->pcmview)) {
        ___queue_decode_file(gvctx, er->file_name);
    }
    /* 
     * This select_row call must be here.
     */
    if (row_select) {
        gtk_clist_select_row(GTK_CLIST(gvctx->editctx.clist),
                             er->row_num, er->col_num); 
    }
}



void ___gtk_box_add_index(GtkBox *box, GtkWidget *child, gint indx)
{
    gtk_box_pack_start(box, child, FALSE, FALSE, 0);
    gtk_box_reorder_child(box, child, indx);
    gtk_widget_show(child);
}


GtkWidget *___make_dialog_informational_title(GtkWidget *widget,
                                        gint cancel_button,
                                        char *file_name,
                                        char *title,
                                        char *msg)
{
    GtkWidget *dialog;
    GtkWidget *label;
    char tmplabel[256];
    char *dialog_title = tmplabel;
    char *cp;

    if (!title || (title == msg)) {
        cp = msg;
        while (*cp && (*cp == '\r' || *cp == '\n')) {
            cp++;
        }
        strncpy(tmplabel, cp, 256);
        tmplabel[255] = '\0';
        cp = strpbrk(tmplabel, "\r\n");
        if (cp) {
            *cp = '\0';
        }
    }
    else  {
        dialog_title = title;
    }
    

    if (cancel_button) {
        dialog = gtk_dialog_new_with_buttons(
                     dialog_title,
                     GTK_WINDOW(widget),
                     GTK_DIALOG_MODAL, 
                     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,
                     NULL);
    }
    else {
        dialog = gtk_dialog_new_with_buttons(
                     dialog_title,
                     GTK_WINDOW(widget),
                     GTK_DIALOG_MODAL, 
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,
                     NULL);
    }

    label = gtk_label_new(msg);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       label, FALSE, FALSE, 0);
    if (file_name) {
        label = gtk_label_new(file_name);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                           label, FALSE, FALSE, 0);
    }
    return dialog;
}


GtkWidget *___make_dialog_informational(GtkWidget *widget,
                                        gint cancel_button,
                                        char *file_name,
                                        char *msg)
{
    return ___make_dialog_informational_title(widget, cancel_button,
                                              file_name, msg, msg);
}


gint ___make_dialog_informational_run_title(global_vars_ctx *gvctx,
                                      gint cancel_button,
                                      char *title,
                                      char *msg)
{
    gint response;
    GtkWidget *dialog;

    dialog = ___make_dialog_informational_title(
                 gvctx->main_window,
                 cancel_button, NULL,
                 title,
                 msg);
    gtk_widget_show_all(dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return response;
}


gint ___make_dialog_informational_run(global_vars_ctx *gvctx,
                                      gint cancel_button,
                                      char *msg)
{
    return ___make_dialog_informational_run_title(gvctx, cancel_button,
                                                  msg, msg);
}



void ___make_cant_open_sound(global_vars_ctx *gvctx)
{
    static char *msg = 
      "                Couldn't Open Sound Device\n\n\n"
      "    Please verify that:\n"
      "      1. No other programs are using the soundcard      \n"
      "      2. Your soundcard is properly configured          \n";


    ___make_dialog_informational_run(gvctx, FALSE, msg);
}


void ___row_modified_dialog_1(global_vars_ctx *gvctx)
{
    gint response;
    char msg[128];
    char *waswere = "time was ";

    cb_stop_button(gvctx->idlectx->player->button_stop, gvctx->idlectx);
    strcpy(msg, "        Current row ");
    if (gvctx->idlectx->dirty || gvctx->idlectx->dirty_saved) {
        strcat(msg, "start ");
    }
    if (gvctx->idlectx->dirty  || gvctx->idlectx->dirty_saved) {
        strcat(msg, "and ");
        waswere = "times were ";
    }
    strcat(msg, waswere);
    strcat(msg, "modified        \n\n\n");
    strcat(msg, "   Record changes?\n");
    response = ___make_dialog_informational_run(gvctx, TRUE, msg);
    if (response == GTK_RESPONSE_OK) {
        gvctx->editctx.cursor = gvctx->idlectx->dirty_row;
        if (gvctx->idlectx->dirty_saved) {
            gvctx->idlectx->elapsed_sec  = gvctx->idlectx->dirty_sec;
            gvctx->idlectx->elapsed_usec = gvctx->idlectx->dirty_usec;
            cb_record_button(gvctx->idlectx->player->button_recstart,
                             (gpointer) gvctx);
            cb_record_button(gvctx->idlectx->player->button_recstop,
                             (gpointer) gvctx);
        }
    }
}


int ___row_modified_idleproc(gpointer data)
{
    global_vars_ctx *gvctx = (global_vars_ctx *) data;

#ifndef WIN32
    if (gdk_pointer_is_grabbed()) {
        /*
         * Do nothing until the cursor is ungrabbed.  This polling is
         * essential as this problem seems to be timing dependent.  Calling
         * this function only once usually works, but sometimes it is called
         * dozens of times. Remove this section at your peril.
         */
        return TRUE;
    }
#endif

    /*
     * Finally, call the modal dialog.  Return FALSE causes the idleproc to
     * quit.
     */
    ___row_modified_dialog_1(gvctx);

    /*
     * When provided, call function that is chained in to follow
     * complete processing of ___row_modified_dialog().
     */
    if (gvctx->row_modified_ctx.proc) {
        gvctx->row_modified_ctx.proc(gvctx->row_modified_ctx.data);
        gvctx->row_modified_ctx.proc = NULL;
    }
    return FALSE;
}


/*
 * This is a work-around for a well-known bug in GTK.  There is an unfortunate
 * interaction between GtkClist when GTK_SELECTION_BROWSE is enabled and a
 * modal dialog that causes the application to loose cursor focus.  This also
 * causes the X server to lock up.  This happens because both the clist and
 * the modal dialog are grabbing the cursor at the same time. The solution
 * implemented here is to start an idle proc to poll until the pointer is
 * ungrabbed.  This is a hack.
 *
 * This thread has the best discussion I've found yet for a work-around to
 * this problem.
 *
 *   http://mail.gnome.org/archives/gtk-list/2000-September/msg00283.html
 *   http://mail.gnome.org/archives/gtk-list/2000-September/msg00296.html
 *   http://mail.gnome.org/archives/gtk-list/2000-September/msg00298.html
 */
void ___row_modified_dialog(global_vars_ctx *gvctx)
{
    gtk_idle_add(___row_modified_idleproc, (gpointer) gvctx);
}


gint ___editor_dirty_dialog_run(global_vars_ctx *gvctx)
{
    gint response;

    response = ___make_dialog_informational_run_title(gvctx, TRUE,
                   "Quit xmpgedit?",
                   "\n\n"
                   "           Current session was changed            \n"
                   "           All modifications will be lost!        \n\n\n"
                   "                Discard changes?\n");
    return response;
                                                
}


void ___load_editor_blank_first_line(global_vars_ctx *gvctx)
{
    char          *dummy_argv[2];

    dummy_argv[0] = "";
    dummy_argv[1] = NULL;
    ___load_editor_by_files(gvctx, &dummy_argv[0], 1);
    gtk_clist_set_text(GTK_CLIST(gvctx->editctx.clist),
        gvctx->editctx.cursor->row_num, 0, "");
    gtk_clist_set_text(GTK_CLIST(gvctx->editctx.clist),
        gvctx->editctx.cursor->row_num, 1, "");
    gtk_clist_set_text(GTK_CLIST(gvctx->editctx.clist),
        gvctx->editctx.cursor->row_num, 2, "");
}


void cb_clear_editor(edit_menu_ctx *emctx)
{
    global_vars_ctx *gvctx;
    edit_row_ctx  *del;
    gint          response;
    edit_ctx      *editctx;

    if (!emctx) {
        return;
    }
    gvctx   = emctx->gvctx;
    editctx = &gvctx->editctx;

    cb_stop_button(gvctx->idlectx->player->button_stop, gvctx->idlectx);
    if (editctx->dirty || gvctx->idlectx->dirty) {
        response = ___editor_dirty_dialog_run(gvctx);
        if (response != GTK_RESPONSE_OK) {
            return;
        }
    }

    gtk_entry_set_text(GTK_ENTRY(gvctx->entry), "");
    gtk_tooltips_disable(gvctx->tooltips_entry);
        
    editctx->cursor = editctx->head;
    do {
        del = ___edit_row_list_delete(editctx);
    } while (del);
    gtk_clist_clear(GTK_CLIST(editctx->clist));
    editctx->dirty = FALSE;
    gtk_widget_set_sensitive(gvctx->menu_save_session, editctx->dirty);

    gtk_adjustment_set_value(
        GTK_ADJUSTMENT(gvctx->idlectx->player->playtime_adj), 0);

    ___set_editor_defaults(gvctx);
    ___load_editor_blank_first_line(gvctx);
    gvctx->ledtrt->settime(gvctx->ledtrt, 0, 0);
    _adjust_ledtrt(gvctx);
    if (GTK_WIDGET_VISIBLE(gvctx->pcmviewctx->pcmview)) {
        gvctx->pcmviewctx->close(gvctx->pcmviewctx);
    }
    cb_offset(gvctx);
}


/*
 * This glue function is not strictly needed, but is used consistency
 * with the paradigm.
 */
void menuitem_response_rmd(void *data)
{
    void menuitem_response(edit_menu_ctx *emctx);

    menuitem_response((edit_menu_ctx *) data);
}


void menuitem_response(edit_menu_ctx *emctx)
{
    edit_row_ctx  *row;
    edit_row_ctx  *row_prev;
    edit_row_ctx  *row_next;
    int           insert;
    GtkAdjustment *adj;
    gdouble       incr;
    char          *item[4];
    global_vars_ctx *gvctx;
    menu_action_e action;
    edit_ctx      *editctx;
    edit_menu_ctx emctx_dup;
    int           del_row_num;
    int           seek_sec;
    long          tmpssec;
    long          tmpesec;
    gint          rsp;

    if (!emctx) {
        return;
    }

    gvctx   = emctx->gvctx;
    action  = emctx->action;
    editctx = &gvctx->editctx;

#ifdef _DEBUG
    printf("menuitem_response %s action=%d\n", emctx->actionstr, action);
#endif
    if (action == EDIT_ACTION_DELETE) {
        if (!editctx->cursor) {
            return;
        }
        row = editctx->cursor;
        if (row->row_num == 0) {
            return;
        }
        if (editctx->cursor && gvctx->idlectx->dirty) {
            gvctx->row_modified_ctx.proc = menuitem_response_rmd;
            gvctx->row_modified_ctx.data = emctx;
            ___row_modified_dialog(gvctx);
            return;
        }
        if (editctx->deleted) {
            ___row_free(editctx->deleted);
        }

        set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, TRUE);
        gvctx->pcmviewctx->set_drawn_flag(gvctx->pcmviewctx, TRUE);

        /*
         * It is very important the cursor is positioned to a different line
         * than the one being removed.  Otherwise, lots of extra callbacks
         * are triggered because the CList widget moves the cursor to the 
         * first line in the list before deleting the current line.
         */
        editctx->dirty   = TRUE;
        del_row_num      = editctx->cursor->row_num;
        editctx->deleted = ___edit_row_list_delete(editctx);
        ___edit_row_set_focus(gvctx, TRUE, TRUE);
        gtk_clist_remove(GTK_CLIST(editctx->clist), del_row_num);

        gtk_widget_set_sensitive(gvctx->menu_edit_pbelow, TRUE);
        gtk_widget_set_sensitive(gvctx->menu_save_session, editctx->dirty);
        return;
    }
    if (action == EDIT_ACTION_PASTE || action == EDIT_ACTION_INSERT) {
        insert = (action == EDIT_ACTION_INSERT) ? 1 : 0;
        if (editctx->deleted) {
            if (gvctx->idlectx->dirty) {
                gvctx->row_modified_ctx.proc = menuitem_response_rmd;
                gvctx->row_modified_ctx.data = emctx;
                ___row_modified_dialog(gvctx);
                return;
            }
            adj = gtk_scrolled_window_get_vadjustment(
                      GTK_SCROLLED_WINDOW(editctx->scrolled_window));
            row = editctx->deleted;
            editctx->deleted = NULL;

            set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, TRUE);
            gvctx->pcmviewctx->set_drawn_flag(gvctx->pcmviewctx, TRUE);

            ___edit_row_list_insert_cursor(row, editctx, insert);
            item[0] = row->file_name;
            item[1] = row->stime_str;
            item[2] = row->etime_str;
            item[3] = "";
            gtk_clist_insert(GTK_CLIST(editctx->clist),
                             editctx->cursor->row_num, item);
            editctx->dirty = TRUE;

            /*
             * Force the column number to 1 when pasting/inserting a row.
             * This leads to confusing behavior unless this is canonicalized
             * to the first column.
             */
            gvctx->editctx.cursor->col_num = 1;

            ___edit_row_set_focus(gvctx, TRUE, TRUE);
            /*
             * Compute the amount the scroll window must be moved by to
             * see the entry after it is pasted in.  This is a bit
             * of a hack at this point.
             */
            incr = adj->upper / ((gdouble) editctx->tail->row_num) + 2.0;
            if (action == EDIT_ACTION_INSERT) {
                incr = -incr;
            }
            gtk_adjustment_set_value(adj, adj->value + incr);
            gtk_scrolled_window_set_vadjustment(
               GTK_SCROLLED_WINDOW(editctx->scrolled_window), adj);
            gtk_widget_set_sensitive(gvctx->menu_edit_pbelow, FALSE);
            gtk_widget_set_sensitive(gvctx->menu_save_session, editctx->dirty);
        }
        return;
    }

    /* Check for uncommitted edit and give chance to save */
    tmpssec = editctx->cursor->s_min*60 + editctx->cursor->s_sec;
    tmpesec = editctx->cursor->e_min*60 + editctx->cursor->e_sec;
    if (action == EDIT_ACTION_CHAIN && 
        !(gvctx->idlectx->elapsed_sec == tmpssec && 
         gvctx->idlectx->elapsed_usec == editctx->cursor->s_msec) &&
        !(gvctx->idlectx->elapsed_sec == tmpesec && 
         gvctx->idlectx->elapsed_usec == editctx->cursor->e_msec))
    {
        rsp = ___make_dialog_informational_run_title(gvctx, TRUE,
                   "Not Committed Warning",
                   "\n"
                   "      OOPS: Current edit is not committed!      \n"
                   "\n"
                   "                               Save change Anyway?      "
                   "\n");
        if (rsp != GTK_RESPONSE_OK) {
            action = 0;
        }
    }


    /* Validate times for common mistakes before taking CHAIN action */
    if (action == EDIT_ACTION_CHAIN && 
        editctx->cursor->s_min  == editctx->cursor->e_min && 
        editctx->cursor->s_sec  == editctx->cursor->e_sec &&
        editctx->cursor->s_msec == editctx->cursor->e_msec)
    {
        rsp = ___make_dialog_informational_run_title(gvctx, TRUE,
                   "Not Edited Error",
                   "\n"
                   "      OOPS: Current row start/end times are equal!      \n"
                   "\n"
                   "                               Save change Anyway?      "
                   "\n");
        if (rsp != GTK_RESPONSE_OK) {
            action = 0;
        }
    }
    else if (action == EDIT_ACTION_CHAIN && 
             ((editctx->cursor->e_min == editctx->cursor->s_min && 
               editctx->cursor->e_sec == editctx->cursor->s_sec &&
               editctx->cursor->e_msec < editctx->cursor->s_msec) ||
              (editctx->cursor->e_min == editctx->cursor->s_min && 
               editctx->cursor->e_sec  < editctx->cursor->s_sec) ||
              (editctx->cursor->e_min  < editctx->cursor->s_min)))
    {
        /* Do not allow this edit, so always set action to "FALSE" */
        ___make_dialog_informational_run_title(gvctx, FALSE,
            "Bad Start Time Error",
            "\n"
            "      OOPS: Current row end time before start time!      \n"
            "\n");
        action = 0;
    }

    /*
     * Copy end time from previous entry to start time of current entry
     */
    if (action == EDIT_ACTION_COPY ||
        action == EDIT_ACTION_DUP  ||
        action == EDIT_ACTION_CHAIN)
    {
        row = editctx->cursor;
        if (row) {
            if (row->row_num == 0) {
                return;
            }
            if (gvctx->idlectx->dirty) {
                gvctx->row_modified_ctx.proc = menuitem_response_rmd;
                gvctx->row_modified_ctx.data = emctx;
                ___row_modified_dialog(gvctx);
                return;
            }
            if (editctx->deleted) {
                ___row_free(editctx->deleted);
            }
            editctx->deleted = ___row_make(row->file_name, 0,
                                           row->s_min, row->s_sec, row->s_msec,
                                           row->e_min, row->e_sec, row->e_msec,
                                           gvctx);
#ifdef _DEBUG
            printf("after delete widget row_num is %d\n", 
                   editctx->cursor->row_num);
#endif
            if (action == EDIT_ACTION_DUP || action == EDIT_ACTION_CHAIN) {
                emctx_dup.action     = EDIT_ACTION_PASTE;
                emctx_dup.gvctx      = gvctx;
                emctx_dup.actionstr  = "Append";
                menuitem_response(&emctx_dup);
                gvctx->pcmviewctx->set_drawn_flag(gvctx->pcmviewctx, TRUE);
                set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, TRUE);
                if (action == EDIT_ACTION_DUP) {
                    return;
                }
            }
            else {
                gtk_widget_set_sensitive(gvctx->menu_edit_pbelow, TRUE);
            }
        }
    }
    if (action == EDIT_ACTION_TIME || action == EDIT_ACTION_CHAIN) {
        /*
         * Copy end time from previous entry to start time of current entry
         */
        if (editctx->cursor->row_num > 1) {
            row                          = editctx->cursor;
            row_prev                     = editctx->cursor->prev;
            gvctx->idlectx->elapsed_sec  = row_prev->e_min*60 +
                                               row_prev->e_sec;
            gvctx->idlectx->elapsed_usec = row_prev->e_msec;

            seek_sec = (gvctx->idlectx->elapsed_sec >= 10) ?
                        gvctx->idlectx->elapsed_sec-10 : 0;
            gvctx->pcmviewctx->seek_sec(gvctx->pcmviewctx, seek_sec);
                                        
            cb_record_button(gvctx->idlectx->player->button_recstart,
                             (gpointer) gvctx);
        }
    }
    else if (action == EDIT_ACTION_STIME) {
        /*
         * Copy start time from next entry to end time of current entry
         */
        if (editctx->cursor->row_num > 0) {
            row                          = editctx->cursor;
            row_next                     = editctx->cursor->next;
            if (row_next) {
                gvctx->idlectx->elapsed_sec  = row_next->s_min*60 +
                                                   row_next->s_sec;
                gvctx->idlectx->elapsed_usec = row_next->s_msec;

                seek_sec = (gvctx->idlectx->elapsed_sec >= 10) ?
                            gvctx->idlectx->elapsed_sec-10 : 0;
                gvctx->pcmviewctx->seek_sec(gvctx->pcmviewctx, seek_sec);
                                        
                cb_record_button(gvctx->idlectx->player->button_recstop,
                                 (gpointer) gvctx);
            }
        }
    }
    else {
        ;
#ifdef _DEBUG
        printf("Unknown action %d\n", action);
#endif
    }
}


void cb_menuitem_chain(GtkWidget *widget, gpointer data)
{
    edit_menu_ctx   emctx;
    global_vars_ctx *gvctx;

    gvctx           = (global_vars_ctx *) data;
    emctx.gvctx     = gvctx;
    emctx.action    = EDIT_ACTION_CHAIN;
    emctx.actionstr = "Copy/Chain";
    set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, TRUE);

    menuitem_response(&emctx);
}


edit_menu_ctx *___make_emctx(global_vars_ctx *gvctx,
                             menu_action_e action,
                             char *actionstr)
{
    edit_menu_ctx *emctx;

    emctx = calloc(1, sizeof(edit_menu_ctx));
    if (!emctx) {
        return NULL;
    }
    actionstr = strdup(actionstr);
    if (!actionstr) {
        free(emctx);
        return NULL;
    }
    emctx->gvctx     = gvctx;
    emctx->action    = action;
    emctx->actionstr = actionstr;
    return emctx;
}


/*
 * Generate a default edit output file name from the first entry in the
 * editor, when no output file has been set.
 */
char *___make_default_output_filename(global_vars_ctx *gvctx)
{
    char *name;
    char *new_name = NULL;
    char *cp;

    if (gvctx->output_file && !*gvctx->output_file && 
        gvctx->editctx.head && gvctx->editctx.head->next)
    {
        name = gvctx->editctx.head->next->file_name;
        cp   = strrchr(name, '/');
        if (cp) {
            name = cp + 1;
        }
        new_name = malloc(strlen(name) + 2);
        if (new_name) {
            strcpy(new_name, name);
        }
    }
    else if (gvctx->output_file) {
        new_name = strdup(gvctx->output_file);
    }
    return new_name;
}


void ___stop_playback(idle_ctx *idlectx)
{
    int      playing;
    int      playto;
    
    playing  = gtk_toggle_button_get_active(
                   GTK_TOGGLE_BUTTON(idlectx->player->button_play));
    playto  = gtk_toggle_button_get_active(
                   GTK_TOGGLE_BUTTON(idlectx->player->button_playto));
    if (playing || playto) {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_pause), FALSE);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
#if 0
        idlectx->playctx = NULL; 
#endif
    }
}


static void ___pcmview_set_values(gpointer data)
{
    mpgedit_pcmview_ctx *pcmviewctx = data;
    global_vars_ctx     *gvctx      = pcmviewctx->get_data(pcmviewctx);
    long                pcmsec  = 0;
    long                pcmmsec = 0;
    idle_ctx            *idlectx;

    if (!gvctx) {
        return;
    }
    idlectx = gvctx->idlectx;
    if (!pcmviewctx->get_data_xpos(pcmviewctx, NULL, &pcmsec, &pcmmsec)) {
        D_printf(("___pcmview_set_values: sec=%ld msec=%ld\n", pcmsec, pcmmsec));
        ___pcmview_draw_cursor_sec(gvctx, 
                                   idlectx->stime_sec, 
                                   idlectx->stime_usec, 100);
        idlectx->elapsed_sec  = pcmsec;
        idlectx->elapsed_usec = pcmmsec;
        ___stop_playback(idlectx);
        return;
    }
    ___spin_button_set_value(&idlectx->player->sbctx, pcmsec, pcmmsec);
    ___scale_set_value(&idlectx->player->sbctx, pcmsec, pcmmsec);
    ___pcmview_draw_cursor_sec(gvctx, pcmsec, pcmmsec, 100);
    idlectx->stime_sec    = pcmsec;
    idlectx->stime_usec   = pcmmsec;
    idlectx->elapsed_sec  = pcmsec;
    idlectx->elapsed_usec = pcmmsec;
    idlectx->gvctx->ledlen->settime(idlectx->gvctx->ledlen, pcmsec, pcmmsec);
    ___stop_playback(idlectx);
#ifdef _DEBUG
    printf("___pcmview_set_values %ld:%02ld.%03ld\n",
            pcmsec/60,
            pcmsec%60,
            pcmmsec);
#endif
}


void cb_pcmview(GtkWidget *widget,
                gpointer data,
                int button,
                gdouble x,
                gdouble y)
{
#ifdef _DEBUG
    printf("cb_pcmview called\n");
#endif
    ___pcmview_set_values(data);
}


void cb_decode_viewfile(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    indexfile_idleproc_ctx *idlectx = data;
    global_vars_ctx        *gvctx;
    mpgedit_pcmview_ctx    *pcmviewctx;
#ifdef _DEBUG
    printf("cb_decode_viewfile: called\n");
#endif

    if (!idlectx || !idlectx->gvctx || !idlectx->ifctx || 
        !idlectx->gvctx->pcmviewctx ||
        !idlectx->gvctx->editctx.cursor->file_name ||
        !idlectx->gvctx->editctx.cursor->file_name[0])
    {
        return;
    }

    gvctx               = idlectx->gvctx;
    pcmviewctx          = gvctx->pcmviewctx;
    pcmviewctx->set_filename(pcmviewctx, idlectx->ifctx->pcmlevelname);
    pcmviewctx->set_data(pcmviewctx, gvctx);
#ifdef _DEBUG
    printf("pcmfile_name = <%s> drawn=%ld, block=%ld\n", pcmviewctx->pcmfile,
           gvctx->pcmviewctx->get_drawn_flag(gvctx->pcmviewctx),
           get_block_pcmview_flag(&gvctx->idlectx->player->sbctx));
#endif

    gtk_widget_show(gvctx->pcmviewctx->pcmview);
    if (!gvctx->pcmviewctx->get_drawn_flag(gvctx->pcmviewctx) &&
        !get_block_pcmview_flag(&gvctx->idlectx->player->sbctx))
    {
        pcmviewctx->draw_event(NULL, pcmviewctx);
    }
#ifdef _DEBUG
else {
    printf("cb_decode_viewfile: draw_event NOT called!\n");
}
#endif
    set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, FALSE);
    gvctx->pcmviewctx->set_drawn_flag(gvctx->pcmviewctx, TRUE);
}


void cb_hide_viewfile(GtkWidget *widget, gpointer data)
{
    global_vars_ctx        *gvctx = (global_vars_ctx *) data;

    gtk_widget_hide(gvctx->pcmviewctx->pcmview);
}


gint decode_file_idleproc(gpointer data)
{
    indexfile_idleproc_ctx *ipctx = (indexfile_idleproc_ctx *) data;
    global_vars_ctx        *gvctx = ipctx->gvctx;

    idle_indexfile_ctx     *ctx;
    long                   tnow;
    gdouble                percent;
    int                    frame_size;
    int                    go;
    unsigned char          *pcmbuf;
    int                    pcmlen;
    int                    bsbytes;
    struct stat            sb;
    int                    i;
    int                    channel_mode = 3;
    int                    init = FALSE;
    int                    pcmmax;
    int                    pcmavg;
    int                    sec;
    int                    msec;
    GtkWidget              *dialog;
    int                    pcm_version;
    int                    pcm_bits;
    int                    pcm_bitssec;
    int                    pcm_bitsmsec;
    int                    pcm_average = 0;
    int                    pcm_avgmax = 0;
    int                    pcm_avgmin = 0;


    if (!ipctx->ifctx) {
        ctx          = ___index_file_list_get(gvctx->decode_queue);
        ipctx->ifctx = ctx;
    }
    else {
        ctx = ipctx->ifctx;
    }

    if (!ctx) {
        /* Delete this idle proc when nothing left to process */
        if (ipctx->dialog) {
            gtk_widget_destroy(ipctx->dialog);
        }
        free(ipctx);
        gvctx->idle_decodefile_count--;
        return FALSE;
    }

    if (!ctx->playctx) {
        if (!ctx->file_name || !ctx->file_name[0]) {
            /* Nothing to do here if there is no file name */
#ifdef _DEBUG
            printf("decode_file_idleproc: bailing... \n");
#endif
            /* Must null out ifctx, otherwise will just loop */
            free(ctx), ipctx->ifctx = NULL;
            return TRUE;
        }
        init = TRUE;
        ctx->pcmmax = -1;
        ctx->pcmmin = 100000;
        ctx->pcmlevelname = mpgedit_index_build_filename_2(ctx->file_name,
                                                           ".lvl");
        if (!ctx->pcmlevelname) {
            /*
             * Fatal error, local directory is read-only, so cannot create
             * .idx or .lvl file here.  Error out.
             */
            dialog = ___make_dialog_informational(gvctx->main_window,
                                                  FALSE,
                                                  (char *) ctx->file_name,
            "        ERROR: Cannot decode file in read-only directory\n\n");
            if (dialog) {
                gtk_widget_show_all(dialog);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
            /* Must null out ifctx, otherwise will just loop */
            free(ctx), ipctx->ifctx = NULL;
            return TRUE;
        }

        ctx->pcmh = mpgedit_pcmlevel_open(ctx->pcmlevelname, "rb");
        if (ctx->pcmh) {
            /*
             * Read header data down to average values.  File is incomplete
             * when average header values are all 0's.
             */
            if (mpgedit_pcmlevel_read_header(ctx->pcmh, &pcm_version,
                &pcm_bits, &pcm_bitssec, &pcm_bitsmsec))
            {
                if (!mpgedit_pcmlevel_read_average(ctx->pcmh, &pcm_average,
                    &pcm_avgmax, &pcm_avgmin))
                {
                    pcm_average = pcm_avgmax = pcm_avgmin = 0;
                }
            }
            else {
                pcm_average = pcm_avgmax = pcm_avgmin = 0;
            }

            if (pcm_average != 0 || pcm_avgmax != 0 || pcm_avgmin != 0) {
                /*
                 * nothing to do here. Trigger callback and all done.
                 */
                mpgedit_pcmlevel_close(ctx->pcmh);
                ctx->pcmh = NULL;
                if (ctx->callback) {
                    ctx->callback(ctx->widget, ctx->event, ctx->data);
                }
                /* Must null out ifctx, otherwise will just loop */
                free(ctx), ipctx->ifctx = NULL;
                return TRUE;
            }
        }

        /*
         * Open decoder; is fine if audio device fails to open. This context
         * is used just mp3->wav decoding, and is not used for audio output.
         */
        ctx->playctx = mpgedit_play_init(ctx->file_name,
                                         MPGEDIT_FLAGS_NO_AUDIODEV);
        if (!ctx->playctx) {
            free(ipctx);
            gvctx->idle_decodefile_count--;
            mpgedit_pcmlevel_close(ctx->pcmh);
            return FALSE;
        }
        ___init_volume(gvctx->volumectx, ctx->playctx);

        ctx->pcmh = mpgedit_pcmlevel_open(ctx->pcmlevelname, "wb");
        if (stat(ctx->file_name, &sb) != -1) {
            ctx->fsize = sb.st_size;
        }

        /*
         * All this to figure out if we are stereo or mono
         */
        mpgedit_play_decode_frame_header_info(
            ctx->playctx, NULL, NULL, NULL, NULL, NULL, NULL,
            &channel_mode, NULL);
        ctx->stereo = (channel_mode != 3);
        mpgedit_pcmlevel_write_header(ctx->pcmh, 1, 16, 22, 10);
        /*
         * placeholder entry, will backfill the correct values later
         */
        mpgedit_pcmlevel_write_average(ctx->pcmh, 0, 0, 0);
    }
    /* if (!ctx->playctx)  */


    if (ipctx->quit) {
        go = 0;
    }
    else {
/* adam/TBD: 8 */
        for (i=0; i<8; i++) {
            mpgedit_play_skip_frame(ctx->playctx);
            mpgedit_play_decode_frame_header_info(
                ctx->playctx, NULL, NULL, NULL, NULL, NULL, 
                &frame_size, NULL, NULL);
                ctx->offset += frame_size;
        }
        go = mpgedit_play_decode_frame(ctx->playctx,
                                       &pcmbuf, &pcmlen, &bsbytes);
    }

    if (go) {
        mpgedit_play_decode_frame_header_info(
            ctx->playctx, NULL, NULL, NULL, NULL, NULL, 
            &frame_size, NULL, NULL);
            ctx->offset += frame_size;
        /*
         * adam/TBD: This is a hack to work around a layer3 decoding
         * problem.  Occasionally, a frame cannot backstep some bits,
         * resulting in a zero level frame. Substitute the last non-zero
         * level frame in  this case.
         */
        if (pcmlen == 0) {
            pcmmax = ctx->pcmmax_last;
        }
        else {
            pcmmax = wav16_samples_max(pcmbuf, pcmlen, ctx->stereo, 0);
            ctx->pcmmax_last = pcmmax;
        }

        /*
         * Compute max/min frame level
         */
        if (pcmmax > ctx->pcmmax) {
            ctx->pcmmax = pcmmax;
        }
        if (pcmmax > 0 && pcmmax < ctx->pcmmin) {
            ctx->pcmmin = pcmmax;
        }

        /*
         * Average computation.
         */
        ctx->pcmavg1 += pcmmax;
        ctx->pcmavg1cnt++;
        if (ctx->pcmavg1cnt > 30000) {
            ctx->pcmavg2 += ctx->pcmavg1 / ctx->pcmavg1cnt;
            ctx->pcmavg2cnt++;
            ctx->pcmavg1    = 0;
            ctx->pcmavg1cnt = 0;
        }

        sec  = mpgedit_play_current_sec(ctx->playctx);
        msec = mpgedit_play_current_msec(ctx->playctx);
        mpgedit_pcmlevel_write_entry(ctx->pcmh, pcmmax, sec, msec);
    }

    if (init) {
        if (go) {
            /*
             * Create dialog box with progress bar
             */
            ipctx->dialog = ___make_indexfile_progress_dialog(
                                ipctx,
                                gvctx->main_window,
                                "Decoding: ",
                                ctx->file_name);
            gtk_widget_show_all(ipctx->dialog);
        }
    }
    else {
        if (!ipctx->quit && go) {
            tnow   = mpgedit_play_current_sec(ctx->playctx);
            if (tnow > ctx->tlast) {
                percent = ((gdouble) ctx->offset) / ((gdouble) ctx->fsize);
                gtk_progress_bar_set_fraction(
                    GTK_PROGRESS_BAR(ipctx->progress), percent);
                ctx->tlast = tnow;
            }
        }
        else {
            if (!ipctx->quit) {
                /*
                 * Write extra entry at end of lvl file with the time of the
                 * last frame.
                 */
                 sec  = mpgedit_play_current_sec(ctx->playctx);
                 msec = mpgedit_play_current_msec(ctx->playctx);
                 mpgedit_pcmlevel_write_entry(ctx->pcmh,
                                              ctx->pcmmax_last, sec, msec);
     
                /*
                 * Finalize average PCM level computation and write to
                 * placeholder position in levels file.
                 */
                if (ctx->pcmavg1cnt) {
                    ctx->pcmavg2 += ctx->pcmavg1 / ctx->pcmavg1cnt;
                }
                pcmavg = ctx->pcmavg2 / (ctx->pcmavg2cnt + 1);
                mpgedit_pcmlevel_seek(ctx->pcmh, 4);
                mpgedit_pcmlevel_write_average(ctx->pcmh, pcmavg,
                                       ctx->pcmmax, ctx->pcmmin);
            }

            mpgedit_play_close(ctx->playctx);        
            mpgedit_pcmlevel_close(ctx->pcmh), ctx->pcmh = NULL;
            ___init_volume(gvctx->volumectx, NULL);

            /*
             * Done with the decoding.  Trigger callback, and then
             * close the context.
             */
            if (ctx->callback) {
                /*
                 * This file has just been decoded, so it has never been
                 * drawn, so reset the pcmview drawn_flag.
                 */
                gvctx->pcmviewctx->set_drawn_flag(gvctx->pcmviewctx, FALSE);
                ctx->callback(ctx->widget, ctx->event, ctx->data);
            }

            /* Must null out ifctx, otherwise will just loop */
            free(ctx), ipctx->ifctx = NULL;
        }
    }
    return TRUE;
}


void ___queue_decode_file(global_vars_ctx *gvctx, char *file)
{
    idle_indexfile_ctx     *ifctx;
    indexfile_idleproc_ctx *idlectx;

    idlectx = (indexfile_idleproc_ctx *)
                  calloc(1, sizeof (indexfile_idleproc_ctx));
    if (!idlectx) {
        return;
    }
    ifctx = (idle_indexfile_ctx *) calloc(1, sizeof(idle_indexfile_ctx));
    if (!ifctx) {
        free(idlectx);
        return;
    }
    idlectx->gvctx   = gvctx;
    ifctx->file_name = strdup(file); /* memory leak ?? */
    ifctx->callback  = cb_decode_viewfile;
    ifctx->data      = (gpointer) idlectx;

    /*
     * decode_file_idleproc() extracts this entry off the
     * list for processing.
     */
    ___index_file_list_put(gvctx->decode_queue, ifctx);

    if (gvctx->idle_decodefile_count < MAX_IDLE_DECODE_PROC) {
        gvctx->idle_decodefile_count++;
        gtk_idle_add(decode_file_idleproc, idlectx);
    }
}


void cb_decode(global_vars_ctx *gvctx)
{
#if defined(_DEBUG)
    printf("cb_decode called\n");
#endif

    if (gvctx && gvctx->stats_str && gvctx->editctx.cursor &&
        gvctx->editctx.cursor->file_name && 
        gvctx->editctx.cursor->file_name[0]) 
    {
#if defined(_DEBUG)
        printf("decoding '%s'\n", gvctx->editctx.cursor->file_name);
#endif
        ___queue_decode_file(gvctx, gvctx->editctx.cursor->file_name);
    }
    else {
        ___make_dialog_informational_run(gvctx, FALSE,
            "\r\n\r\n   Error: You must open a file first   \r\n\r\n");
    }
}


void cb_statistics(global_vars_ctx *gvctx)
{
#if defined(_DEBUG)
    printf("cb_statistics called\n");
#endif
    if (gvctx && gvctx->stats_str) {
        ___make_dialog_informational_run_title(gvctx, FALSE, 
                                               "File Statistics",
                                               gvctx->stats_str);
    }
    else {
        ___make_dialog_informational_run(gvctx, FALSE,
            "\r\n\r\n   Error: You must open a file first   \r\n\r\n");
    }
}


void _adjust_ledtrt(global_vars_ctx *gvctx)
{
    long offset_sec, offset_msec;

    mpeg_time_gettime(&gvctx->offset_time, &offset_sec, &offset_msec);
    if (!gvctx) {
        return;
    }

    gvctx->ledtrt->sec -= offset_sec;
    gvctx->ledtrt->msec -= offset_msec;
    if (gvctx->ledtrt->msec < 0) {
        gvctx->ledtrt->msec += 1000;
        gvctx->ledtrt->sec--;
        if (gvctx->ledtrt->sec < 0) {
            gvctx->ledtrt->sec = 0;
            gvctx->ledtrt->msec = 0;
        }
    }
    gvctx->ledtrt->settime(gvctx->ledtrt,
                           gvctx->ledtrt->sec,
                           gvctx->ledtrt->msec);
    mpeg_time_init(&gvctx->trt_time, 
                   gvctx->ledtrt->sec,
                   gvctx->ledtrt->msec);
}



void cb_offset(global_vars_ctx *gvctx)
{
#if defined(_DEBUG)
    printf("cb_offset called\n");
#endif

    mpeg_time_init(&gvctx->offset_time, 
                   gvctx->idlectx->elapsed_sec, 
                   gvctx->idlectx->elapsed_usec);
    gvctx->ledoffset->settime(gvctx->ledoffset, 
                              gvctx->idlectx->elapsed_sec,
                              gvctx->idlectx->elapsed_usec);
    if (gvctx->pcmviewctx) {
        gvctx->pcmviewctx->set_offset(gvctx->pcmviewctx,
                                      gvctx->idlectx->elapsed_sec,
                                      gvctx->idlectx->elapsed_usec);
    }

    _adjust_ledtrt(gvctx);
}


void mainmenu_item_response(edit_menu_ctx *emctx)
{
    global_vars_ctx *gvctx = emctx->gvctx;

#ifdef _DEBUG
    printf("mainmenu_item_response: action=%d\n", emctx->action);
#endif
    switch (emctx->action) {
      case MAINMENU_ACTION_OPEN:
        cb_open_file_from_menu(emctx->gvctx);
        break;

      case MAINMENU_ACTION_OUTFILE:
        gvctx->ofdialogctx = ___make_dialog_outputfile(gvctx,
                                                      "Output file name",
                                                      TRUE,
                                                      &gvctx->output_file,
                                                      &gvctx->output_action);
        g_signal_connect(GTK_OBJECT (gvctx->ofdialogctx->dialog), 
                         "response", 
                         G_CALLBACK(cb_dialog_outputfile),
                         (gpointer) gvctx->ofdialogctx);

        break;

      case MAINMENU_ACTION_LOAD_ABANDONED:
        gvctx->abandon_file_ctx =
            ___make_dialog_outputfile_title(gvctx,
                                      "Load Saved Session",
                                      "\n    Abandoned edits file name    ",
                                      FALSE,
                                      &gvctx->abandon_file,
                                      NULL);
        g_signal_connect(GTK_OBJECT(gvctx->abandon_file_ctx->dialog), 
                         "response", 
                         G_CALLBACK(cb_dialog_outputfile),
                         (gpointer) gvctx->abandon_file_ctx);
        g_signal_connect(GTK_OBJECT(gvctx->abandon_file_ctx->dialog),
                         "response", 
                         G_CALLBACK(cb_load),
                         (gpointer) gvctx);
        break;

      case MAINMENU_ACTION_SAVE_ABANDON:
        emctx->gvctx->edit_action_abandon_save = TRUE;
        cb_edit_1(NULL, emctx->gvctx);
        break;

      case MAINMENU_ACTION_EDIT:
        cb_edit_1(NULL, emctx->gvctx);
        break;

      case MAINMENU_ACTION_DECODE:
        cb_decode(emctx->gvctx);
        break;

      case MAINMENU_ACTION_STATS:
        cb_statistics(emctx->gvctx);
        break;

      case MAINMENU_ACTION_OFFSET:
        cb_offset(emctx->gvctx);
        break;

      case MAINMENU_ACTION_CLEAR:
        cb_clear_editor(emctx);
        break;

      case MAINMENU_ACTION_ABOUT:
        gtk_widget_show_all(emctx->gvctx->about_dialog);
        gtk_dialog_run(GTK_DIALOG(emctx->gvctx->about_dialog));
        gtk_widget_hide_all(emctx->gvctx->about_dialog);
        break;

      default:
        printf("oops: mainmenu_item_response case not handled %d\n",
                emctx->action);
        break;
    }
}


void optionmenu_item_response(GtkWidget *widget, edit_menu_ctx *emctx)
{
    int state = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    int state_change = 0;
    global_vars_ctx *gvctx = emctx->gvctx;

#ifdef _DEBUG
    printf("optionmenu_item_response: action=%d state=%d\n",
           emctx->action, state);
#endif
    switch (emctx->action) {
      case OPTIONMENU_ACTION_STOP:
        if (state != gvctx->stop_visable) {
            state_change = TRUE;
            gvctx->stop_visable = state;
        }
        if (state) {
            gtk_widget_show(gvctx->idlectx->player->button_stop);
        }
        else {
            gtk_widget_hide(gvctx->idlectx->player->button_stop);
        }
        break;

      case OPTIONMENU_ACTION_PAUSE:
        if (state != gvctx->pause_visable) {
            state_change                = TRUE;
            gvctx->pause_visable = state;
        }
        if (state) {
            gtk_widget_show(gvctx->idlectx->player->button_pause);
        }
        else {
            gtk_widget_hide(gvctx->idlectx->player->button_pause);
        }
        break;

      case OPTIONMENU_ACTION_VOLUME:
        if (state != gvctx->volume_visable) {
            state_change                = TRUE;
            gvctx->volume_visable = state;
        }
        if (state) {
            gtk_widget_show(gvctx->volumectx->box);
        }
        else {
            gtk_widget_hide(gvctx->volumectx->box);
        }
        break;

      case OPTIONMENU_ACTION_OFFSET:
        if (state != gvctx->offset_visable) {
            state_change          = TRUE;
            gvctx->offset_visable = state;
        }
        if (state) {
            gtk_widget_show(gvctx->frame_offset);
            gtk_widget_show(gvctx->menu_offset);
            gtk_widget_show(gvctx->menu_offset_sep);
        }
        else {
            gtk_widget_hide(gvctx->frame_offset);
            gtk_widget_hide(gvctx->menu_offset);
            gtk_widget_hide(gvctx->menu_offset_sep);
        }
        break;

      default:
        printf("oops: optionmenu_item_response case not handled %d\n",
                emctx->action);
        break;
    }

    if (state_change) {
        write_config_file(CONFIG_FILE_NAME, gvctx);
    }
}



GtkWidget *___optionmenu_make(global_vars_ctx *gvctx)
{
    GtkWidget *menu;
    GtkWidget *menu_stop;
    GtkWidget *menu_pause;
    GtkWidget *menu_vol;
    GtkWidget *menu_offset;
    edit_menu_ctx *omctx;

    menu         = gtk_menu_new();
    menu_stop    = gtk_check_menu_item_new_with_label("Stop button");
    menu_pause   = gtk_check_menu_item_new_with_label("Pause button");
    menu_vol     = gtk_check_menu_item_new_with_label("Volume Control");
    menu_offset  = gtk_check_menu_item_new_with_label("Offset Time");

    gtk_widget_show(menu_stop);
    gtk_widget_show(menu_pause);
    gtk_widget_show(menu_vol);
    gtk_widget_show(menu_offset);

    gtk_menu_append(GTK_MENU(menu), menu_stop);
    gtk_menu_append(GTK_MENU(menu), menu_pause);
    gtk_menu_append(GTK_MENU(menu), menu_vol);
    gtk_menu_append(GTK_MENU(menu), menu_offset);

    omctx = ___make_emctx(gvctx, OPTIONMENU_ACTION_STOP, "Stop");
    g_signal_connect(G_OBJECT(menu_stop), "activate",
                     G_CALLBACK(optionmenu_item_response), omctx);
    omctx = ___make_emctx(gvctx, OPTIONMENU_ACTION_PAUSE, "Pause");
    g_signal_connect(G_OBJECT(menu_pause), "activate", 
                     G_CALLBACK(optionmenu_item_response), omctx);
    omctx = ___make_emctx(gvctx, OPTIONMENU_ACTION_VOLUME, "Volume");
    g_signal_connect(G_OBJECT(menu_vol), "activate", 
                     G_CALLBACK(optionmenu_item_response), omctx);
    omctx = ___make_emctx(gvctx, OPTIONMENU_ACTION_OFFSET, "Offset");
    g_signal_connect(G_OBJECT(menu_offset), "activate", 
                     G_CALLBACK(optionmenu_item_response), omctx);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_stop),
                                   gvctx->stop_visable);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_pause),
                                   gvctx->pause_visable);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_vol),
                                   gvctx->volume_visable);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_offset),
                                   gvctx->offset_visable);
    return menu;
}


GtkWidget *___editmenu_make(global_vars_ctx *gvctx)
{
    GtkWidget     *menu;
    edit_menu_ctx *emctx;

    GtkWidget *menu_clear;
    GtkWidget *menu_edit_copy;
    GtkWidget *menu_edit_cut;
    GtkWidget *menu_edit_pbelow;
    GtkWidget *menu_edit_chain;
    GtkWidget *menu_edit_duplicate;
    GtkWidget *menu_edit_stime;
    GtkWidget *menu_edit_time;
    GtkWidget *menu_separator;
    GtkWidget *menu_separator2;
    GtkWidget *menu_separator3;

    menu                = gtk_menu_new();
    menu_edit_cut       = gtk_menu_item_new_with_label("Cut");
    menu_edit_copy      = gtk_menu_item_new_with_label("Copy");
    menu_edit_pbelow    = gtk_menu_item_new_with_label("Append");
    menu_separator      = gtk_separator_menu_item_new();
    menu_edit_chain     = gtk_menu_item_new_with_label("Copy/Chain");
    menu_edit_duplicate = gtk_menu_item_new_with_label("Copy/Append");
    menu_separator2     = gtk_separator_menu_item_new();
    menu_edit_stime     = gtk_menu_item_new_with_label("Copy Start Time");
    menu_edit_time      = gtk_menu_item_new_with_label("Copy End Time");
    menu_separator3     = gtk_separator_menu_item_new();
    menu_clear          = gtk_menu_item_new_with_label("Clear editor");

    gtk_widget_show(menu_edit_cut);
    gtk_widget_show(menu_edit_copy);
    gtk_widget_show(menu_edit_pbelow);
    gtk_widget_show(menu_separator);
    gtk_widget_show(menu_edit_chain);
    gtk_widget_show(menu_edit_duplicate);
    gtk_widget_show(menu_separator2);
    gtk_widget_show(menu_edit_stime);
    gtk_widget_show(menu_edit_time);
    gtk_widget_show(menu_separator3);
    gtk_widget_show(menu_clear);

    gtk_menu_append(GTK_MENU(menu), menu_edit_cut);
    gtk_menu_append(GTK_MENU(menu), menu_edit_copy);
    gtk_menu_append(GTK_MENU(menu), menu_edit_pbelow);
    gtk_menu_append(GTK_MENU(menu), menu_separator);
    gtk_menu_append(GTK_MENU(menu), menu_edit_chain);
    gtk_menu_append(GTK_MENU(menu), menu_edit_duplicate);
    gtk_menu_append(GTK_MENU(menu), menu_separator2);
    gtk_menu_append(GTK_MENU(menu), menu_edit_stime);
    gtk_menu_append(GTK_MENU(menu), menu_edit_time);
    gtk_menu_append(GTK_MENU(menu), menu_separator3);
    gtk_menu_append(GTK_MENU(menu), menu_clear);

    emctx = ___make_emctx(gvctx, EDIT_ACTION_DELETE, "Delete");
    g_signal_connect_swapped(G_OBJECT(menu_edit_cut), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, EDIT_ACTION_COPY, "Copy");
    g_signal_connect_swapped(G_OBJECT(menu_edit_copy), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, EDIT_ACTION_PASTE, "Append");
    g_signal_connect_swapped(G_OBJECT(menu_edit_pbelow), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, EDIT_ACTION_CHAIN, "Copy/Chain");
    g_signal_connect_swapped(G_OBJECT(menu_edit_chain), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, EDIT_ACTION_DUP, "Copy/Append");
    g_signal_connect_swapped(G_OBJECT(menu_edit_duplicate), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, EDIT_ACTION_STIME, "Copy Start Time");
    g_signal_connect_swapped(G_OBJECT(menu_edit_stime), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, EDIT_ACTION_TIME, "Copy End Time");
    g_signal_connect_swapped(G_OBJECT(menu_edit_time), "activate", 
                             G_CALLBACK(menuitem_response), emctx);
    emctx = ___make_emctx(gvctx, MAINMENU_ACTION_CLEAR, "Clear editor");
    g_signal_connect_swapped(G_OBJECT(menu_clear), "activate", 
                             G_CALLBACK(mainmenu_item_response), emctx);

    gtk_widget_set_sensitive(menu_edit_pbelow,    FALSE);
    gtk_widget_set_sensitive(menu_edit_cut,       FALSE);
    gtk_widget_set_sensitive(menu_edit_copy,      FALSE);
    gtk_widget_set_sensitive(menu_edit_chain,     FALSE);
    gtk_widget_set_sensitive(menu_edit_duplicate, FALSE);
    gtk_widget_set_sensitive(menu_edit_stime,     FALSE);
    gtk_widget_set_sensitive(menu_edit_time,      FALSE);

    gvctx->menu_edit_pbelow    = menu_edit_pbelow;
    gvctx->menu_edit_cut       = menu_edit_cut;
    gvctx->menu_edit_copy      = menu_edit_copy;
    gvctx->menu_edit_chain     = menu_edit_chain;
    gvctx->menu_edit_duplicate = menu_edit_duplicate;
    gvctx->menu_edit_stime     = menu_edit_stime;
    gvctx->menu_edit_time      = menu_edit_time;
    return menu;
}


GtkWidget *___helpmenu_make(global_vars_ctx *gvctx)
{
    GtkWidget *menu;
    GtkWidget *menu_about;
    edit_menu_ctx *mmctx;

    menu           = gtk_menu_new();
    menu_about     = gtk_menu_item_new_with_label("About");

    gtk_widget_show(menu_about);

    gtk_menu_append(GTK_MENU(menu), menu_about);

/* probably wrong callback action for this menu */
    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_ABOUT, "About");
    g_signal_connect_swapped(G_OBJECT(menu_about), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);
    return menu;
}


GtkWidget *___mainmenu_make(global_vars_ctx *gvctx)
{
    GtkWidget *menu;
    GtkWidget *menu_open;
    GtkWidget *menu_edit;
    GtkWidget *menu_decode;
    GtkWidget *menu_stats;
    GtkWidget *menu_separator1;
    GtkWidget *menu_separator2;
    GtkWidget *menu_separator3;
    GtkWidget *menu_separator3a;
    GtkWidget *menu_separator3b;
    GtkWidget *menu_separator4;
    GtkWidget *menu_separator5;
    GtkWidget *menu_options;
    GtkWidget *menu_outfile;
    GtkWidget *menu_exit;
    GtkWidget *menu_load_abandoned;
    GtkWidget *option_menu;
    edit_menu_ctx *mmctx;

    menu            = gtk_menu_new();
    menu_open       = gtk_menu_item_new_with_label("Open File...");
    menu_separator5 = gtk_separator_menu_item_new();
    menu_outfile    = gtk_menu_item_new_with_label("Output file...");
    menu_separator1 = gtk_separator_menu_item_new();

    gvctx->menu_save_session = 
                     gtk_menu_item_new_with_label("Save session...");
    menu_load_abandoned =
                     gtk_menu_item_new_with_label("Load session...");
    menu_separator2 = gtk_separator_menu_item_new();

    menu_edit       = gtk_menu_item_new_with_label("Edit");
    menu_separator3 = gtk_separator_menu_item_new();

    gvctx->menu_offset = gtk_menu_item_new_with_label("Set Offset");
    gvctx->menu_offset_sep = gtk_separator_menu_item_new();

    menu_decode      = gtk_menu_item_new_with_label("Decode");
    menu_separator3a = gtk_separator_menu_item_new();

    menu_stats       = gtk_menu_item_new_with_label("Statistics...");
    menu_separator3b = gtk_separator_menu_item_new();

    menu_options    = gtk_menu_item_new_with_label("Options");
    menu_separator4 = gtk_separator_menu_item_new();

    menu_exit       = gtk_menu_item_new_with_label("Exit");

    gtk_widget_show(menu_open);
    gtk_widget_show(menu_separator5);
    gtk_widget_show(menu_outfile);
    gtk_widget_show(menu_separator1);
    gtk_widget_show(gvctx->menu_save_session);
    gtk_widget_show(menu_load_abandoned);
    gtk_widget_show(menu_separator2);
    gtk_widget_show(menu_edit);
    gtk_widget_show(menu_separator3);
    gtk_widget_show(menu_decode);
    gtk_widget_show(menu_separator3a);
    gtk_widget_show(menu_stats);
    gtk_widget_show(menu_separator3b);
    gtk_widget_show(menu_options);
    gtk_widget_show(menu_separator4);
    gtk_widget_show(menu_exit);

    gtk_menu_append(GTK_MENU(menu), menu_open);
    gtk_menu_append(GTK_MENU(menu), menu_separator5);
    gtk_menu_append(GTK_MENU(menu), menu_outfile);
    gtk_menu_append(GTK_MENU(menu), menu_separator1);
    gtk_menu_append(GTK_MENU(menu), gvctx->menu_save_session);
    gtk_menu_append(GTK_MENU(menu), menu_load_abandoned);
    gtk_menu_append(GTK_MENU(menu), menu_separator2);
    gtk_menu_append(GTK_MENU(menu), menu_edit);
    gtk_menu_append(GTK_MENU(menu), menu_separator3);
    gtk_menu_append(GTK_MENU(menu), gvctx->menu_offset);
    gtk_menu_append(GTK_MENU(menu), gvctx->menu_offset_sep);
    gtk_menu_append(GTK_MENU(menu), menu_decode);
    gtk_menu_append(GTK_MENU(menu), menu_separator3a);
    gtk_menu_append(GTK_MENU(menu), menu_stats);
    gtk_menu_append(GTK_MENU(menu), menu_separator3b);
    gtk_menu_append(GTK_MENU(menu), menu_options);
    gtk_menu_append(GTK_MENU(menu), menu_separator4);
    gtk_menu_append(GTK_MENU(menu), menu_exit);

    gtk_widget_set_sensitive(gvctx->menu_save_session, FALSE);

    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_OPEN,
                          "Open file");
    g_signal_connect_swapped(G_OBJECT(menu_open), "activate",
                             G_CALLBACK(mainmenu_item_response), mmctx);
    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_LOAD_ABANDONED,
                          "Load session");
    g_signal_connect_swapped(G_OBJECT(menu_load_abandoned), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);
    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_SAVE_ABANDON,
                          "Save edit session");
    g_signal_connect_swapped(G_OBJECT(gvctx->menu_save_session), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);
    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_OUTFILE, "Output file");
    g_signal_connect_swapped(G_OBJECT(menu_outfile), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);

    option_menu = ___optionmenu_make(gvctx);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_options),
                              option_menu);
    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_EDIT, "Edit");
    g_signal_connect_swapped(G_OBJECT(menu_edit), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);

    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_DECODE, "Decode");
    g_signal_connect_swapped(G_OBJECT(menu_decode), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);

    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_STATS, "Statistics");
    g_signal_connect_swapped(G_OBJECT(menu_stats), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);

    mmctx = ___make_emctx(gvctx, MAINMENU_ACTION_OFFSET, "Offset");
    g_signal_connect_swapped(G_OBJECT(gvctx->menu_offset), "activate", 
                             G_CALLBACK(mainmenu_item_response), mmctx);

    g_signal_connect(G_OBJECT(menu_exit), "activate",
                     G_CALLBACK(cb_quit), gvctx->idlectx);

    gvctx->menu_edit = menu_edit;

    gtk_widget_set_sensitive(menu_edit, FALSE);
    return menu;
}


GtkWidget *___aboutdialog_make(global_vars_ctx *gvctx)
{
    GtkWidget *dialog;
    GtkWidget *label1;
    GtkWidget *label2;
    GtkWidget *scrolled_window;


    dialog = gtk_dialog_new_with_buttons(
                 "About",
                 GTK_WINDOW(gvctx->main_window), GTK_DIALOG_MODAL,
                 GTK_STOCK_OK,     GTK_RESPONSE_OK,
                 NULL);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    label1 = gtk_label_new( 
      "                     " XMPGEDIT_PRODUCT "\n\n"
                            MPGEDIT_COPYRIGHT);
    label2 = gtk_label_new(
      "Product:         " XMPGEDIT_PRODUCT "\n"
      "Version:         " XMPGEDIT_VERSION " " MPGEDIT_URL"\n"
      "Build:            " MPGEDIT_BUILDNUM "("MPGEDIT_BUILDOS")\n"
      "Report bugs to:  " MPGEDIT_MAILTO "\n\n"
      XMPGEDIT_CREDITS_COPYRIGHT);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       label1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       scrolled_window, FALSE, FALSE, 5);

    gtk_scrolled_window_add_with_viewport(
                   GTK_SCROLLED_WINDOW(scrolled_window), label2);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, -1, 200);
    return dialog;
}


void cb_set_pcmview_gnomon(global_vars_ctx *gvctx, long sec, long msec)
{
    ___pcmview_draw_cursor_sec(gvctx, sec, msec, 10);
}


int cb_label_set_text(void *data, long sec, long usec)
{
    long offset_sec, offset_msec;

    idle_ctx *idlectx = (idle_ctx *) data;
    mpeg_time_gettime(&idlectx->gvctx->offset_time, &offset_sec, &offset_msec);
    sec  -= offset_sec;
    usec -= offset_msec;
    idlectx->elapsed_sec  = sec;
    idlectx->elapsed_usec = usec;
    if (idlectx->saved_sec != sec) {
        idlectx->saved_sec  = sec;
        idlectx->saved_usec = usec;
        idlectx->gvctx->ledlen->settime(idlectx->gvctx->ledlen,
                                        sec, usec);
        cb_set_pcmview_gnomon(idlectx->gvctx, sec, usec);
    }
    return 1;
}


gint idle_play(gpointer data)
{
    idle_ctx            *idlectx = (idle_ctx *) data;
    mpgedit_pcmview_ctx *pcmctx = idlectx->gvctx->pcmviewctx;
    long sec;
    long msec;
    gint go = TRUE;
    long offset_sec;
    long offset_msec;

    sec  = mpgedit_play_current_sec(idlectx->playctx);
    mpeg_time_gettime(&idlectx->gvctx->offset_time, &offset_sec, &offset_msec);
    if (mpgedit_play_frame(idlectx->playctx) == 0) {
        go   = FALSE;
        msec = mpgedit_play_current_msec(idlectx->playctx);
        /*
         * This is a hack, but needed due to how playback is stopped
         * when the time is modified by cb_scrollbar_adj().  There should
         * be a better way to stop playback, other than just calling
         * mpgedit_play_close().  Anyway, it is kind of difficult to get
         * a non-zero sec/msec value after play_close has been called.
         */
        if (sec) {
            sec -= offset_sec;
            idlectx->elapsed_sec = sec;
        }
        if (msec) {
            msec -= offset_msec;
            idlectx->elapsed_usec = msec;
        }
        mpgedit_play_close(idlectx->playctx);
        idlectx->playctx = NULL; 
        ___init_volume(idlectx->gvctx->volumectx, NULL);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
    }
    else {
        /*
         * scroll window right when last second has been displayed
         */
        if (sec > pcmctx->pcm_seclast + offset_sec) {
            pcmctx->seek_sec(pcmctx, sec - offset_sec);
        }
    }
    return go;
}


gint idle_playto(gpointer data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    long sec;
    long msec;
    long offset_sec;
    long offset_msec;

    mpeg_time_gettime(&idlectx->gvctx->offset_time, &offset_sec, &offset_msec);
    sec  = mpgedit_play_current_sec(idlectx->playctx);
    msec = mpgedit_play_current_msec(idlectx->playctx);
    if ((sec  > (idlectx->stime_sec + offset_sec)) || 
        (sec == (idlectx->stime_sec + offset_sec) &&
         msec >= (idlectx->stime_usec + offset_msec)) ||
        (mpgedit_play_frame(idlectx->playctx) == 0))
    {
        idlectx->elapsed_sec  = idlectx->stime_sec;
        idlectx->elapsed_usec = idlectx->stime_usec;
        mpgedit_play_close(idlectx->playctx);
        idlectx->playctx = NULL; 
        ___init_volume(idlectx->gvctx->volumectx, NULL);
        gtk_idle_remove(idlectx->idle_tag);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
    }
    return TRUE;
}


void cb_record_time(GtkWidget *widget, gpointer data)
{
    record_button_ctx_t *btnctx = (record_button_ctx_t *) data;
    global_vars_ctx *gvctx = (global_vars_ctx *) btnctx->gvctx;
    idle_ctx        *idlectx;
    long            sec;
    long            usec;
    int             changed;
    edit_ctx        *editctx;

    if (gvctx->editctx.cursor->row_num == 0) {
        return;
    }
    editctx = &gvctx->editctx;
    sec  = gvctx->idlectx->elapsed_sec;
    usec = gvctx->idlectx->elapsed_usec;
    gvctx->idlectx->save_stime_sec = sec;
    gvctx->idlectx->save_stime_usec = usec;
    

    idlectx = gvctx->idlectx;
    if (btnctx->start) {
        changed = ___row_init(gvctx->editctx.cursor, NULL, 
                              sec/60, sec%60, usec,
                              -1, -1, -1);
    }
    else {
        changed = ___row_init(gvctx->editctx.cursor, NULL, 
                              -1, -1, -1,
                              sec/60, sec%60, usec);
    }
    idlectx->stime_sec  = sec;
    idlectx->stime_usec = usec;

    idlectx->dirty = idlectx->dirty_saved = FALSE;
    /*
     * Nothing has changed, so just quit this callback
     */
    if (!changed) {
        return;
    }
    ___row_update(&gvctx->editctx, gvctx->editctx.clist);
    gvctx->editctx.dirty = TRUE;
    gtk_widget_set_sensitive(gvctx->menu_save_session, gvctx->editctx.dirty);
}


void cb_record_button(GtkWidget *widget, gpointer data)
{
/* adam/TBD: "data" should be idlectx or eidlectx, not gvctx. This will
 * eliminate the need to "decode" which context is needing changing.
 */
    global_vars_ctx *gvctx = (global_vars_ctx *) data;
    idle_ctx        *idlectx;
    long            sec;
    long            usec;
    int             changed;

    idlectx             = gvctx->idlectx;
    idlectx->stime_sec  = sec  = idlectx->elapsed_sec;
    idlectx->stime_usec = usec = idlectx->elapsed_usec;

    /*
     * Must manipulate playback controls separately.  Both are available
     * in the gvctx. The first is the start time control, the other is
     * the end time control.
     */
    if (gvctx->idlectx->player->button_recstart == widget) {
/*
 * This sucks!! Need reference to the portion of the row labels in the 
 * idlectx that applies to the playback control.
 */
        changed = ___row_init(gvctx->editctx.cursor, NULL, 
                              sec/60, sec%60, usec,
                              -1, -1, -1);
    }
    else {
        changed = ___row_init(gvctx->editctx.cursor, NULL, 
                              -1, -1, -1,
                              sec/60, sec%60, usec);
    }

    idlectx->dirty = idlectx->dirty_saved = FALSE;
    /*
     * Nothing has changed, so just quit this callback
     */
    if (!changed) {
        return;
    }
    ___spin_button_set_value(&idlectx->player->sbctx,
                             idlectx->stime_sec,
                             idlectx->stime_usec);

    ___row_update(&gvctx->editctx, gvctx->editctx.clist);
    gvctx->editctx.dirty = TRUE;
    gtk_widget_set_sensitive(gvctx->menu_save_session, gvctx->editctx.dirty);
}


/*
 * Play from 5 seconds before current start time to the currently selected
 * start time.  This callback is similar to cb_play_button(), but uses a
 * different start time criteria, and uses a different idle function,
 * idle_playto(), to do the playback.  idle_playto() stops when the play
 * runtime exceeds the start time.
 */
void cb_playto_button(GtkWidget *widget, gpointer data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    mpgedit_pcmview_ctx *pcmctx = idlectx->gvctx->pcmviewctx;
    long sec;
    long seek_sec;
    long usec;
    int playing;
    int rsec;
    int rmsec;

    /*
     * Quit if play is running
     */
    playing = gtk_toggle_button_get_active(
                  GTK_TOGGLE_BUTTON(idlectx->player->button_play));
    if (playing) {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
        return;
    }

    playing = gtk_toggle_button_get_active(
                  GTK_TOGGLE_BUTTON(idlectx->player->button_playto));
    if (playing) {

        /*
         * Transitioned from stopped -> playing
         */
        if (!idlectx->play_file || 
            (idlectx->stime_sec == 0 && idlectx->stime_usec == 0))
        {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
            return;
        }

        /*
         * Seek to desired play start time. For this button, that is
         * start playing 5 seconds before the current start time.
         */
        sec  = idlectx->stime_sec - PLAYTO_BACKUP_SEC;
        usec = idlectx->stime_usec;
        if (sec < 0) {
            sec  = 0;
            usec = 0;
        }

        idlectx->playctx = mpgedit_play_init(idlectx->play_file, 0);
        if (!idlectx->playctx) {
            ___make_cant_open_sound(idlectx->gvctx);
            /*
             * Do something more exciting when this fails
             */
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
            return;
        }
        ___init_volume(idlectx->gvctx->volumectx, idlectx->playctx);

        idlectx->save_stime_sec  = idlectx->stime_sec;
        idlectx->save_stime_usec = idlectx->stime_usec;
        ___add_offset_time(0, sec, usec,
                           mpeg_time_getsec(&idlectx->gvctx->offset_time),
                           mpeg_time_getusec(&idlectx->gvctx->offset_time), 
                           &rsec, &rmsec);
        mpgedit_play_seek_time(idlectx->playctx, rsec, rmsec);

#ifdef _DEBUG
        printf("cb_playto_button: size=%ld sec=%ld usec=%ld\n", 
               mpgedit_play_total_size(idlectx->playctx),
               mpgedit_play_total_sec(idlectx->playctx),
               mpgedit_play_total_msec(idlectx->playctx));
#endif

        /*
         * scroll window left when last second has been displayed
         */
        if (sec < pcmctx->pcm_secfirst) {
            seek_sec = idlectx->stime_sec - 
                           (pcmctx->pcm_seclast - pcmctx->pcm_secfirst) +
                            PLAYTO_BACKUP_SEC;
            if (seek_sec < 0) {
                seek_sec = 0;
            }
            pcmctx->seek_sec(pcmctx, seek_sec);
            ___pcmview_draw_cursor_sec(idlectx->gvctx,
                                       idlectx->stime_sec, usec, 100);
        }

        idlectx->idle_tag = gtk_timeout_add(PLAY_IDLE_MSEC, idle_playto, idlectx);
        mpgedit_play_set_status_callback(idlectx->playctx,
                                         cb_label_set_text, 
                                         idlectx);
    }
    else {
        /*
         * Transitioned from playing -> stopped
         */
        idlectx->stime_sec    = idlectx->save_stime_sec;
        idlectx->stime_usec   = idlectx->save_stime_usec;
        idlectx->elapsed_sec  = idlectx->stime_sec;
        idlectx->elapsed_usec = idlectx->stime_usec;
        idlectx->gvctx->ledlen->settime(idlectx->gvctx->ledlen, 
                                        idlectx->stime_sec,
                                        idlectx->stime_usec);
        mpgedit_play_close(idlectx->playctx);
        idlectx->playctx = NULL; 
        ___init_volume(idlectx->gvctx->volumectx, NULL);
        if (idlectx->idle_tag) {
            gtk_idle_remove(idlectx->idle_tag);
        }
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_pause), FALSE);
    }
    return;
}


void ___read_time_from_spin_buttons(idle_ctx *idlectx, long *sec, long *msec)
{
    long tsec;

    tsec =  (long) gtk_adjustment_get_value(idlectx->player->sbctx.adj_min);
    tsec *= 60;
    tsec += (long) gtk_adjustment_get_value(idlectx->player->sbctx.adj_sec);

    *sec  = tsec;
    *msec = (long) gtk_adjustment_get_value(idlectx->player->sbctx.adj_msec);
}


void cb_scrollbar_adj(GtkAdjustment *adj, gpointer data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    long     sec;
    long     msec;

#ifdef _DEBUG
    printf("cb_scrollbar_adj: called %ld\n",
           get_block_pcmview_flag(&idlectx->player->sbctx)); 
#endif
    ___read_time_from_spin_buttons(idlectx, &sec, &msec);
    ___stop_playback(idlectx);
    if (!idlectx->adj_elapsed_label_noset) {
        idlectx->gvctx->ledlen->settime(idlectx->gvctx->ledlen, sec, msec);
    }
    idlectx->stime_sec    = sec;
    idlectx->elapsed_sec  = sec;
    idlectx->stime_usec   = msec;
    idlectx->elapsed_usec = msec;

    if (!get_block_pcmview_flag(&idlectx->player->sbctx)) {
        idlectx->gvctx->pcmviewctx->seek_sec(idlectx->gvctx->pcmviewctx, sec);
    }
}


/*
 * Convenience function to initialize the player contexts when the 
 * file selection widget OK button is pushed.
 * Inputs:
 *   idlectx->play_file
 */
void ___file_sel_ok(GtkWidget *widget, gpointer data,
                    long init_sec, long init_msec)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    void     *playctx = NULL;
    long     sec;
    long     msec;
    int      playing;

    /*
     * Stop playing file if playing
     */
    playing  = gtk_toggle_button_get_active(
                   GTK_TOGGLE_BUTTON(idlectx->player->button_play));
    if (playing) {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_pause), FALSE);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
        idlectx->playctx = NULL; 
    }
    playing  = gtk_toggle_button_get_active(
                   GTK_TOGGLE_BUTTON(idlectx->player->button_playto));
    if (playing) {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_pause), FALSE);
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
        idlectx->playctx = NULL; 
    }

    /*
     * Set the playback scroll bar range using the input file play length
     */
    playctx = mpgedit_play_init(idlectx->play_file, 0);
    if (!playctx) {
        return;
    }
    ___init_volume(idlectx->gvctx->volumectx, playctx);

    sec  = mpgedit_play_total_sec(playctx);
    msec = mpgedit_play_total_msec(playctx);
    mpgedit_play_close(playctx);
    idlectx->playctx = NULL;
    ___init_volume(idlectx->gvctx->volumectx, NULL);
    if (!idlectx->adj_elapsed_label_noset) {
        idlectx->gvctx->ledtrt->settime(idlectx->gvctx->ledtrt, sec, msec);
    }
    _adjust_ledtrt(idlectx->gvctx);

    ___set_scale_spin_buttons_range_from_time(idlectx->player, sec, msec);
    if (init_sec || init_msec) {
        ___spin_button_set_value(&idlectx->player->sbctx, init_sec, init_msec);
    }
    else {
        gtk_adjustment_set_value(
            GTK_ADJUSTMENT(idlectx->player->playtime_adj), 0.0);
    }

    if (!idlectx->adj_elapsed_label_noset) {
        idlectx->gvctx->ledlen->settime(idlectx->gvctx->ledlen,
                                        idlectx->elapsed_sec,
                                        idlectx->elapsed_usec);
    }
}


void cb_edit_menu_clist(GtkWidget *widget,
                    int row,
                    int column,
                    GdkEventButton *event,
                    gpointer data)
{
    GdkEventButton  *event_button = (GdkEventButton *) event;
    global_vars_ctx *gvctx        = (global_vars_ctx *) data;

#ifdef _DEBUG
    printf("cb_edit_menu_clist: event type=%d event_button=%d\n", 
           event ? event->type : -1,
           (event && event->type == GDK_BUTTON_PRESS) ?
                                    event_button->button : -1);
#endif
    if (event && event->type == GDK_BUTTON_PRESS) {
        if (event_button->button == 3) {
            gtk_menu_popup (GTK_MENU (gvctx->edit_menu), NULL, NULL, NULL,
                            NULL, event_button->button, event_button->time);

        }
    }
}


void cb_row_clicked_2(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    edit_row_ctx    *er      = (edit_row_ctx *) data;
    global_vars_ctx *gvctx   = (global_vars_ctx *) er->ctx;
#ifdef _DEBUG
    printf("cb_row_clicked_2: called\n");
#endif

    gvctx->idlectx->dirty  = FALSE;
    gvctx->editctx.cursor  = er;
    if (er->row_num == 0) {
        gvctx->pcmviewctx->close(gvctx->pcmviewctx);
    }
    else if (GTK_WIDGET_VISIBLE(gvctx->pcmviewctx->pcmview)) {
        /* adam/TBD: memory allocation issues here */
        gvctx->pcmviewctx->set_filename(gvctx->pcmviewctx,
            mpgedit_index_build_filename_2(er->file_name, ".lvl"));
        gvctx->pcmviewctx->set_data(gvctx->pcmviewctx, gvctx);
#ifdef _DEBUG
        printf("pcmfile_name = <%s>\n", gvctx->pcmviewctx->pcmfile);
#endif
    
        /* adam/TBD: Need cooresponding hide call */
        gtk_widget_show(gvctx->pcmviewctx->pcmview);

        if (!get_block_pcmview_flag(&gvctx->idlectx->player->sbctx)) {
            gvctx->pcmviewctx->draw_event(NULL, gvctx->pcmviewctx);
        }
        set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, FALSE);
        gvctx->pcmviewctx->set_drawn_flag(gvctx->pcmviewctx, TRUE);
    }
    ___edit_row_set_focus(gvctx, TRUE, TRUE);
}


void cb_row_clicked_1_rmd(gpointer data)
{
    edit_row_ctx    *er      = (edit_row_ctx *)    data;
    global_vars_ctx *gvctx   = (global_vars_ctx *) er->ctx;

    cb_row_clicked_2(gvctx->row_modified_ctx.widget,
                     gvctx->row_modified_ctx.event,
                     data);
}


void cb_row_clicked_1(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    edit_row_ctx    *er      = (edit_row_ctx *) data;
    global_vars_ctx *gvctx   = (global_vars_ctx *) er->ctx;

#ifdef _DEBUG
        printf("cb_row_clicked_1: called\n");
#endif
    if (gvctx->editctx.cursor && gvctx->idlectx->dirty) {
#ifdef _DEBUG
        printf("cb_row_clicked_1: stime=%d:%d s=%d:%d.%d e=%d:%d.%d\n",
               gvctx->idlectx->stime_sec, 
               gvctx->idlectx->stime_usec, 
               er->s_min, er->s_sec, er->s_msec, 
               er->e_min, er->e_sec, er->e_msec);
#endif
        gvctx->row_modified_ctx.proc   = cb_row_clicked_1_rmd;
        gvctx->row_modified_ctx.data   = data;
        gvctx->row_modified_ctx.widget = widget;
        gvctx->row_modified_ctx.event  = event;
        ___row_modified_dialog(gvctx);
        return;
    }

    cb_row_clicked_2(widget, event, data);
}


idle_indexfile_ctx *___index_file_list_get(mpgedit_workqueue_t *q)
{
    index_file_list *node;
    idle_indexfile_ctx *ifctx;

    if (!q || !q->ifl_head) {
        return NULL;
    }

    node        = q->ifl_head;
    q->ifl_head = q->ifl_head->next;
    if (!q->ifl_head) {
        q->ifl_tail = NULL;
    }
    
    ifctx = node->ifctx;
    free(node);

    return ifctx;
}


int ___index_file_list_put(mpgedit_workqueue_t *q, idle_indexfile_ctx *ifctx)
{
    index_file_list *node;

    if (!q || !ifctx) {
        return 1;
    }

    node = (index_file_list *) calloc(1, sizeof(index_file_list));
    if (!node) {
        return 1;
    }
    node->ifctx = ifctx;
    if (!q->ifl_head) {
        q->ifl_head = node;
        q->ifl_tail = node;
    }
    else {
        q->ifl_tail->next = node;
        q->ifl_tail       = node;
    }
    return 0;
}


/*
 * Idle proc that performs mp3 file indexing.  This is initialized by
 * the cb_row_clicked() callback.  Once this indexing operation is
 * complete, the remainder of the callback function, cb_row_clicked_1()
 * is called.
 */
gint row_clicked_idle_indexfile(gpointer data)
{
    indexfile_idleproc_ctx *ipctx = (indexfile_idleproc_ctx *) data;
    global_vars_ctx        *gvctx = ipctx->gvctx;
    idle_indexfile_ctx     *ctx;
    long                   tnow;
    gdouble                percent;
    long                   offset;
    char                   *label;
    struct stat            sb;
    GtkWidget              *dialog;
    int                    rsts;
    char                   *stats_str;

#ifdef _DEBUG
    printf("row_clicked_idle_indexfile: called drawn=%ld\n",
            gvctx->pcmviewctx->get_drawn_flag(gvctx->pcmviewctx));
#endif
    if (!ipctx->ifctx) {
        ctx          = ___index_file_list_get(gvctx->index_queue);
        ipctx->ifctx = ctx;
    }
    else {
        ctx = ipctx->ifctx;
    }

    if (!ctx || !ctx->edit_row) {
        /* Delete this idle proc when nothing left to process */
        if (ipctx->dialog) {
            gtk_widget_destroy(ipctx->dialog);
        }
        free(ipctx);
        gvctx->idle_indexfile_count--;
        return 0;
    }

    /*
     * This condition is true when cb_edit_1a() creates a "fake" edit_row
     * context.  The important value here is the callback.
     */
    if (!ctx->file_name && ctx->callback) {
        if (ctx->callback) {
            ctx->callback(ctx->widget, ctx->event, ctx->data);
        }
        free(ctx);
        ipctx->ifctx = NULL;
        return 1;
    }

    if (!ctx->playctx) {
        if (ctx->file_name && ctx->file_name[0] &&
            stat(ctx->file_name, &sb) != -1) 
        {
            ctx->fsize = sb.st_size;
        }
        if (ctx->edit_row->file_name[0]) {
            ctx->playctx = mpgedit_edit_index_new(
                               (char *) ctx->edit_row->file_name, &rsts);
            if (rsts == _MPGEDIT_INDEX_NEW_ERR_DIR_RDONLY) {
                dialog = ___make_dialog_informational(gvctx->main_window,
                                                      FALSE,
                                                      (char *) ctx->file_name,
                             "        ERROR: Cannot initialize "
                             "index context; read-only directory?\n\n");
                if (dialog) {
                    gtk_widget_show_all(dialog);
                    gtk_dialog_run(GTK_DIALOG(dialog));
                    gtk_widget_destroy(dialog);
                }
                free(ctx), ipctx->ifctx = NULL;
                return 1;
            }
        }
        if (rsts == MPGEDIT_EDIT_INDEX_STS_INDEX_EXISTS) {
            ipctx->eof = TRUE;
            return 1;
        }
        else {
            ipctx->eof = mpgedit_edit_index(ctx->playctx);
        }
        if (!ipctx->eof && !ipctx->dialog) {
            /*
             * Create dialog box with progress bar
             */
            ipctx->dialog = ___make_indexfile_progress_dialog(
                                ipctx,
                                gvctx->main_window,
                                "Indexing: ",
                                ctx->file_name);
            gtk_widget_show_all(ipctx->dialog);
        }
        if (ipctx->label) {
            label = malloc(sizeof("Indexing: ") + strlen(ctx->file_name));
            if (label) {
                strcpy(label, "Indexing: ");
                strcat(label, ctx->file_name);
                gtk_label_set_text(GTK_LABEL(ipctx->label), label);
                free(label);
            }
        }
    }
    else if (!ipctx->eof) {
        ipctx->eof = mpgedit_edit_index(ctx->playctx);
    }
    if (!ipctx->quit && !ipctx->eof) {
        tnow   = mpgedit_edit_index_sec(ctx->playctx);
        if (tnow > ctx->tlast) {
            offset = mpgedit_edit_index_offset(ctx->playctx);
            percent = ((gdouble) offset) / ((gdouble) ctx->fsize);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ipctx->progress), 
                                          percent);
            ctx->tlast = tnow + 10;
        }
    }
    else {
        if (ctx->callback) {
#ifdef _DEBUG
            printf("row_clicked_idle_indexfile: calling callback\n");
#endif
            ctx->callback(ctx->widget, ctx->event, ctx->data);
        }

        if (ctx->playctx) {
            /* Possible buffer over run risk here */
            stats_str = malloc(1024);
            mpgedit_edit_index_get_stats(ctx->playctx);
            mpgedit_edit_stats2str(ctx->playctx,
                mpgedit_edit_total_length_time(ctx->playctx), stats_str);
            if (gvctx->stats_str) {
                free(gvctx->stats_str);
            }
            gvctx->stats_str = strdup(stats_str);
            free(stats_str);
        }

        mpgedit_edit_index_free(ctx->playctx);        
        ctx->edit_row->indexing = FALSE;
        free(ctx);
        ipctx->ifctx            = NULL;
    }
    return 1;
}

     
gint cb_indexfile_delete_event(
         GtkWidget *widget, GdkEvent *event, gpointer data)
{
    indexfile_idleproc_ctx *ctx = (indexfile_idleproc_ctx *) data;
    ctx->quit = TRUE;
    return TRUE;
}


/*
 * Callback on the indexfile dialog.  There is a cancel button on the
 * index dialog.  Note: Cancelling indexing does not cancel the need to
 * create the file index.  The next time the file is clicked on, it will
 * be indexed again, until indexing is complete.
 */
void cb_indexfile_dialog_quit(GtkWidget *widget, gint response, gpointer data)
{
    indexfile_idleproc_ctx *ctx = (indexfile_idleproc_ctx *) data;

    if (response == GTK_RESPONSE_CANCEL) {
        ctx->quit = TRUE;
    }
}


void cb_editfile_dialog_quit(GtkWidget *widget, gint response, gpointer data)
{
    idle_editfile_ctx *ctx = (idle_editfile_ctx *) data;

    if (response == GTK_RESPONSE_CANCEL) {
        ctx->quit = TRUE;
    }
}


GtkWidget *___make_indexfile_progress_dialog(indexfile_idleproc_ctx *ctx,
                                             GtkWidget *parent,
                                             char *label_name,
                                             char *file_name)
{
    GtkWidget *dialog;
    GtkWidget *progress;
    GtkWidget *label;
    char      *str;

    str = malloc(PATH_MAX);
    if (!str) {
        return NULL;
    }

    strcpy(str, label_name);
    strcat(str, file_name);
    dialog = gtk_dialog_new_with_buttons(
                 str, GTK_WINDOW(parent), GTK_DIALOG_MODAL,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 NULL);

    g_signal_connect(G_OBJECT(dialog), "response", 
                     G_CALLBACK(cb_indexfile_dialog_quit), ctx);
    g_signal_connect(G_OBJECT(dialog), "delete_event", 
                     G_CALLBACK(cb_indexfile_delete_event), ctx);
                     
    progress      = gtk_progress_bar_new();
    ctx->progress = progress;
    gtk_widget_show(progress);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       progress, FALSE, FALSE, 0);

    label      = gtk_label_new(str);
    ctx->label = label;
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       label, FALSE, FALSE, 0);

    free(str);
    return dialog;
}


GtkWidget *___make_editfile_progress_dialog(idle_editfile_ctx *ctx,
                                            GtkWidget *parent,
                                            char *file_name,
                                            int secs)
{
    GtkWidget *dialog;
    GtkWidget *progress;
    GtkWidget *label;
    GtkWidget *total_time;
    char      *str;
    char      time_str[64];
    

    str = malloc(PATH_MAX);
    if (!str) {
        return NULL;
    }

    sprintf(str, "Editing: %s", file_name);
    sprintf(time_str, "Edit length: %d:%02d:%02d (%ds)",
            secs/3600, (secs%3600)/60, (secs%3600)%60, secs);
    dialog = gtk_dialog_new_with_buttons(
                 str, GTK_WINDOW(parent), GTK_DIALOG_MODAL,
                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                 NULL);

    g_signal_connect(G_OBJECT(dialog), "response", 
                     G_CALLBACK(cb_editfile_dialog_quit), ctx);
                     
    progress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       progress, FALSE, FALSE, 0);

    label = gtk_label_new(str);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       label, FALSE, FALSE, 0);

    total_time = gtk_label_new(time_str);
    gtk_widget_show(total_time);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       total_time, FALSE, FALSE, 0);

    ctx->progress = progress;
    gtk_widget_show(progress);

    free(str);
    return dialog;
}


/*
 * Determine if the index file must be created before processing the 
 * row clicked callback. mpgedit_edit_index_init() is called, and
 * when a context handle is returned, an idle proc is created to 
 * finish the indexing operation.  Once that processing is completed
 * by row_clicked_idle_indexfile(), callback(widget, event, ER) is
 * called.
 */
void ___indexfile_init(GtkWidget *widget, GdkEvent *event, edit_row_ctx *er,
                       void (*callback)(GtkWidget *, GdkEvent *, gpointer))
{
    global_vars_ctx        *gvctx;
    idle_indexfile_ctx     *ifctx;
    indexfile_idleproc_ctx *idlectx;

#ifdef _DEBUG
    printf("___indexfile_init: called: indexing=%d\n", er->indexing);
#endif
    if (er->indexing) {
        return;
    }
    idlectx = (indexfile_idleproc_ctx *)
                  calloc(1, sizeof (indexfile_idleproc_ctx));
    if (!idlectx) {
        return;
    }
    ifctx = (idle_indexfile_ctx *) calloc(1, sizeof(idle_indexfile_ctx));
    if (!ifctx) {
        free(idlectx);
        return;
    }
    gvctx            = (global_vars_ctx *) er->ctx;
    idlectx->gvctx   = gvctx;
    er->indexing     = TRUE;
    ifctx->widget    = widget;
    ifctx->event     = event;
    ifctx->data      = (gpointer) er;
    ifctx->file_name = er->file_name;
    ifctx->edit_row  = er;
    ifctx->callback  = callback;

    /*
     * row_clicked_idle_indexfile() extracts this entry off the
     * list for processing.
     */
    ___index_file_list_put(gvctx->index_queue, ifctx);

    if (gvctx->idle_indexfile_count < MAX_IDLE_INDEX_PROC) {
        gvctx->idle_indexfile_count++;
        gtk_idle_add(row_clicked_idle_indexfile, idlectx);
    }
}


void cb_row_clicked(GtkWidget *widget,
                    int row,
                    int column,
                    GdkEventButton *event,
                    gpointer data)
{
    edit_row_ctx *er;
    global_vars_ctx *gvctx = (global_vars_ctx *) data;

#ifdef _DEBUG
    printf("cb_row_clicked: row=%d column=%d er=%p event=%p\n",
           row, column, er, event);
#endif
   if (event && event->button == 3) {
       gtk_menu_popup(GTK_MENU (gvctx->edit_menu), NULL, NULL, NULL,
                      NULL, event->button, event->time);
       return;
    }

    er = ___edit_row_find_index(&gvctx->editctx, row);
    if (er) {
        er->col_num = column;
        ___indexfile_init(widget, (GdkEvent *) event, er, cb_row_clicked_1);
        gtk_widget_set_sensitive(gvctx->menu_edit_cut,       row>0);
        gtk_widget_set_sensitive(gvctx->menu_edit_copy,      row>0);
        gtk_widget_set_sensitive(gvctx->menu_edit_chain,     row>0);
        gtk_widget_set_sensitive(gvctx->menu_edit_duplicate, row>0);
        gtk_widget_set_sensitive(gvctx->menu_edit_stime,
                                 row>0 && gvctx->editctx.cursor->next);
        gtk_widget_set_sensitive(gvctx->menu_edit_time,      row>1);
        gtk_widget_set_sensitive(gvctx->menu_edit,
                                 (gvctx->editctx.head != gvctx->editctx.tail));
    }
}



void cb_file_sel_ok(GtkWidget *widget, global_vars_ctx *gvctx)
{
    edit_row_ctx    *edit_row;
    GtkAdjustment   *adj;
    gdouble         incr;
    char            *item[4];

    ___file_sel_ok(widget, (gpointer) gvctx->idlectx,  0, 0);
    if (gvctx->editctx.head) {
        gvctx->editctx.dirty = TRUE;
    }
    edit_row = ___row_make(gvctx->idlectx->play_file, 0, 
                           0, 0, 0, 
                           0, 0, 0, gvctx);
    ___edit_row_list_append(edit_row, &gvctx->editctx);

    adj = gtk_scrolled_window_get_vadjustment(
              GTK_SCROLLED_WINDOW(gvctx->editctx.scrolled_window));
    incr = adj->upper / ((gdouble) gvctx->editctx.tail->row_num) + 2.0;
    gtk_adjustment_set_value(adj, adj->value + incr);
    item[0] = edit_row->file_name;
    item[1] = edit_row->stime_str;
    item[2] = edit_row->etime_str;
    item[3] = "0:00:000";
    gtk_clist_append(GTK_CLIST(gvctx->editctx.clist), item);
    gtk_clist_select_row(GTK_CLIST(gvctx->editctx.clist), edit_row->row_num, 0);
    gvctx->editctx.cursor = edit_row;
    gtk_widget_set_sensitive(gvctx->menu_save_session, gvctx->editctx.dirty);
}


int ___test_is_valid_file_error_dialog(const char *name, global_vars_ctx *gvctx)
{
    struct stat sbuf;
    int sts;
    GtkWidget *dialog;

    sts = stat(name, &sbuf);
    if (sts == 0 && !S_ISREG(sbuf.st_mode)) {
        sts = 1;
    }
/*
 * Should do even better here, and test if file is an MPEG audio file
 */
    if (sts) {
        dialog = ___make_dialog_informational(gvctx->main_window,
                     FALSE,
                     (char *) name,
                     "        ERROR: Invalid filename specified        \n\n");
        if (dialog) {
            gtk_widget_show_all(dialog);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
        }
    }
    return sts;
}


void cb_file_dialog_ok(global_vars_ctx *gvctx)
{
    char            *name;
    char            *cp;

    name = gvctx->output_file;

    /*
     * Map .idx->.mp3, since it is not valid to open an index file
     */
    cp = strstr(name, ".idx");
    if (cp && strlen(cp) == 4) {
        strcpy(cp, ".mp3");
    }

    /*
     * Test specified file name for validity.
     */
    if (___test_is_valid_file_error_dialog(name, gvctx)) {
        gtk_entry_set_text(GTK_ENTRY(gvctx->entry), "");
        return;
    }

    if (gvctx->idlectx->play_file) {
        free(gvctx->idlectx->play_file);
    }

    /*
     * Convert backslashes in path to '/'
     */
    cp = name;
    while (*cp) {
        if (*cp == '\\') {
            *cp = '/';
        }
        cp++;
    }

    gvctx->idlectx->play_file  = name;
    gtk_tooltips_enable(gvctx->tooltips_entry);
    gtk_tooltips_set_tip(gvctx->tooltips_entry, GTK_WIDGET(gvctx->entry),
                         name, NULL);

    gtk_entry_set_text(GTK_ENTRY(gvctx->entry), name);
    gtk_editable_set_position(
        GTK_EDITABLE(gvctx->entry), strlen(name));

    /* Ick..., passing NULL for widget parameter may be dangerous. */
    cb_file_sel_ok(NULL, gvctx);
}



void cb_entry(GtkWidget *widget, void *data)
{
    global_vars_ctx *gvctx = (global_vars_ctx *) data;
    const char      *name;
    char            *cp;

    name = strtrim(gtk_entry_get_text(GTK_ENTRY(widget)));
#ifdef _DEBUG
    printf("cb_entry: called '%s'\n", name);
#endif
    /*
     * Map .idx->.mp3, since it is not valid to open an index file
     */
    cp = strstr(name, ".idx");
    if (cp && strlen(cp) == 4) {
        strcpy(cp, ".mp3");
    }
    if (___test_is_valid_file_error_dialog(name, gvctx)) {
        gtk_entry_set_text(GTK_ENTRY(widget), "");
        return;
    }
    if (gvctx->idlectx->play_file) {
        free(gvctx->idlectx->play_file);
    }
    gvctx->idlectx->play_file  = (char *) name;
    cb_file_sel_ok(widget, data);
}


void cb_open_file_from_menu(gpointer data)
{
    cb_open_file(NULL, data);
}


void cb_open_file(GtkWidget *widget, gpointer data)
{
    global_vars_ctx *gvctx = (global_vars_ctx *) data;
    gint            fs_response;
    GtkWidget       *fs;
    char            *ptr;
    int             len;

    fs = gtk_file_selection_new("Open file select");
    gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fs));

    if (!gvctx->filesel_pattern) {
        gvctx->filesel_pattern = malloc(6);
        if (gvctx->filesel_pattern) {
            strcpy(gvctx->filesel_pattern, "*.mp3");
        }
    }
    else {
        /* Stomp any end of line termination found in string */
        ptr = (ptr = strstr(gvctx->filesel_pattern, "\n")) ? ptr :
              (ptr = strstr(gvctx->filesel_pattern, "\r")) ? ptr : NULL;
        if (ptr) {
            *ptr = '\0';
        }
    }
    if (gvctx->filesel_path && gvctx->filesel_path[0]) {
        /* Stomp any end of line termination found in string */
        ptr = (ptr = strstr(gvctx->filesel_path, "\n")) ? ptr :
              (ptr = strstr(gvctx->filesel_path, "\r")) ? ptr : NULL;
        if (ptr) {
            *ptr = '\0';
        }
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs),
                                        gvctx->filesel_path);
    }

    fs_response = gtk_dialog_run(GTK_DIALOG(fs));
#ifdef _DEBUG
    printf("cb_dialog_outputfile: Response code = %d\n", fs_response);
#endif
    if (fs_response == GTK_RESPONSE_OK) {
        gvctx->output_file =
            strtrim(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));
        ptr = strrchr(gvctx->output_file, _DIR_SEPARATOR_CHAR);
        if (ptr) {
            /*
             * Remember the current directory of the file selected.
             */
            if (gvctx->filesel_path) {
                free(gvctx->filesel_path);
            }
            len = strlen(gvctx->output_file) +
                  strlen(gvctx->filesel_pattern) + 2;
            gvctx->filesel_path = malloc(len);
            if (gvctx->filesel_path) {
                strcpy(gvctx->filesel_path, gvctx->output_file);
                ptr = strrchr(gvctx->filesel_path, _DIR_SEPARATOR_CHAR);
                ptr++;
                *ptr = '\0';
                strcat(ptr, gvctx->filesel_pattern);
            }
        }
    }
    gtk_widget_destroy(fs);

    if (fs_response == GTK_RESPONSE_OK) {
        cb_file_dialog_ok(gvctx);
    }
}


gint cb_delete(GtkWidget *widget,
               GdkEvent  *event,
               gpointer   data )
{
    idle_ctx *idlectx = (idle_ctx *) data;
    int playing = gtk_toggle_button_get_active(
                      GTK_TOGGLE_BUTTON(idlectx->player->button_play));

    if (playing) {
        mpgedit_play_close(idlectx->playctx);
        idlectx->playctx = NULL; 
        ___init_volume(idlectx->gvctx->volumectx, NULL);
    }

    /* Write config values, as last directory opened probably changed */
    write_config_file(CONFIG_FILE_NAME, idlectx->gvctx);

    gtk_main_quit();
    return FALSE;
}


void ___list_iterator_func(void *ctx, edit_row_ctx *row)
{
    cb_edit_ctx     *edctx = (cb_edit_ctx *) ctx;
    char            timespec[128];
    int             s_sec_tmp;
    int             s_msec_tmp;
    int             e_sec_tmp;
    int             e_msec_tmp;
    long            offset_sec;
    long            offset_msec;



    mpeg_time_gettime(&edctx->gvctx->offset_time, &offset_sec, &offset_msec);
    if (row->file_name && row->file_name[0]) {

        /* Add offset time to start and end times when set */
        ___add_offset_time(row->s_min, row->s_sec, row->s_msec, 
                           offset_sec, offset_msec,
                           &s_sec_tmp, &s_msec_tmp);
    
        /* Preserve copying to 0:0.0 end time means copy to EOF */
        if (edctx->index == edctx->len &&
            row->e_sec == 0 && row->e_msec == 0)
        {
            e_sec_tmp  = 0;
            e_msec_tmp = 0;
        }
        else {
            ___add_offset_time(row->e_min, row->e_sec, row->e_msec, 
                               offset_sec, offset_msec, 
                               &e_sec_tmp, &e_msec_tmp);
        }
    
        sprintf(timespec, "%d:%d.%d-%d:%d.%d",
                s_sec_tmp/60, s_sec_tmp%60, s_msec_tmp,
                e_sec_tmp/60, e_sec_tmp%60, e_msec_tmp);
                
        mpgedit_editspec_append(edctx->edarray, row->file_name, timespec);
    }
    edctx->index++;
}


long ___get_file_total_secs(char *file)
{
    void *ctx;
    long rsecs = -1;

    ctx = mpgedit_play_init(file, 0);
    if (ctx) {
        rsecs = mpgedit_play_total_sec(ctx);
        mpgedit_play_close(ctx);
    }
    return rsecs;
}


long ___compute_edit_total_secs(
    global_vars_ctx *gvctx,
    editspec_t *edarray,
    int edlen)
{
    long      ssec;
    long      susec;
    long      esec;
    long      eusec;
    char      *filename;
    long rtsecs = 0;
    long diffsecs;
    long esecs;
    int  i;

    for (i=0; i<edlen; i++) {
        mpeg_time_gettime(mpgedit_editspec_get_stime(edarray, i),
                          &ssec, &susec);
        mpeg_time_gettime(mpgedit_editspec_get_etime(edarray, i),
                          &esec, &eusec);
        filename = mpgedit_editspec_get_file(edarray, i);
        if (esec == MP3_TIME_INFINITE && eusec == 0) {
            esecs = ___get_file_total_secs(filename) -
                        mpeg_time_getsec(&gvctx->offset_time);
        }
        else {
            esecs = esec;
        }
        diffsecs = esecs - ssec;
        if (diffsecs > 0) {
            rtsecs += diffsecs;
        }
    }
    return rtsecs;
}


#ifdef _DEBUG
static void debug_print_editspec(editspec_t *edarray)
{
    editspec_t *edentry;
    int        indx;
    int        len;
    int        arraylen;
    int        i;
    char       *filename;
    long       ssec;
    long       susec;
    long       esec;
    long       eusec;

    arraylen = mpgedit_editspec_get_length(edarray);
    indx = 0;
    while (indx<arraylen) {
        edentry = mpgedit_editspec_get_edit(edarray, indx,
                                            arraylen, 0, &len);
        {
            for (i=0; i<len; i++) {
                filename = mpgedit_editspec_get_file(edentry, i);
                mpeg_time_gettime(mpgedit_editspec_get_stime(edentry, i),
                                  &ssec, &susec);
                mpeg_time_gettime(mpgedit_editspec_get_etime(edentry, i),
                                  &esec, &eusec);
                printf("%s (%ld:%ld-%ld:%ld)\n",
                       filename, ssec, susec, esec, eusec);
            }
            printf("\n");
        }
        indx += len;
    }
}
#endif


gint cb_edit_idle_editfile(gpointer data)
{
    idle_editfile_ctx *ctx = (idle_editfile_ctx *) data;
    gint              rval = TRUE;
    int               sts = 0;
    int               built_len;
    gdouble           percent;
    GtkWidget         *dialog;
    int               append    = FALSE;
    int               *indexptr = NULL;
    char              *filename;
    long              ssec;
    long              susec;
    long              esec;
    long              eusec;
    char              *cp;

    eusec = esec = susec = ssec = 0;
    filename = NULL;
    if (ctx->gvctx->output_action == OUTPUT_FILE_APPEND_JOIN) {
        append = TRUE;
    }
    else {
        indexptr = &ctx->out_fileindx;
    }


    
    /*
     * Iterate over the split operation.  Must first initialize the
     * context values, then perform the remaining edit operations
     * iteratively.  Since there are two loops being maintained,
     * a state machine is used to keep everything straight.
     */
    switch (ctx->state) {
      case EDIT_IDLE_STATE_INIT:
        ctx->edit_indx    = 0;
        memset(&ctx->edit_stats, 0, sizeof(ctx->edit_stats));
        ctx->edit_flags = append ? MPGEDIT_FLAGS_APPEND : 0;
        ctx->out_fileindx = 1;
        ctx->state = EDIT_IDLE_STATE_LOOP;
#ifdef _DEBUG
        debug_print_editspec(ctx->edarray);
#endif
        break;

      case EDIT_IDLE_STATE_LOOP:
        if (ctx->edit_indx >= ctx->edarray_len) {
            ctx->state = EDIT_IDLE_STATE_DONE;
            break;
        }
        ctx->current_sec   = 0;
        if (ctx->built_edarray) {
            free(ctx->built_edarray);
        }
        ctx->built_edarray =  
            mpgedit_editspec_get_edit(ctx->edarray, ctx->edit_indx,
                                      ctx->edarray_len, append, &built_len);

        if (!ctx->built_edarray) {
            /* Malloc error */
            ctx->edit_indx = ctx->edarray_len;
            ctx->state     = EDIT_IDLE_STATE_DONE;
            break;
        }

        if (!ctx->gvctx->output_file || !*ctx->gvctx->output_file) {
            ___make_dialog_informational_run(
                ctx->gvctx, FALSE, 
                "ERROR: Output filename must be specified");
                ctx->state = EDIT_IDLE_STATE_DONE;
                break;
        }
        if (!append) {
            cp = ___make_default_output_filename(ctx->gvctx);
            if (cp) {
                if (ctx->gvctx->output_file) {
                    free(ctx->gvctx->output_file);
                }
                ctx->gvctx->output_file = cp;
            }
            
        }
        ctx->out_filename = mpgedit_edit_make_output_filename(
                                NULL,
                                ctx->gvctx->output_file,
                                "mp3",
                                indexptr);
#if defined(_DEBUG)
        {
            int i;
            printf("starting edits %d output=%s\n",
                   ctx->edit_indx, ctx->out_filename);
            for (i=0; i<built_len; i++) {
                filename = mpgedit_editspec_get_file(ctx->built_edarray, i);
                mpeg_time_gettime(mpgedit_editspec_get_stime(
                                      ctx->built_edarray, i),
                                  &ssec, &susec);
                mpeg_time_gettime(mpgedit_editspec_get_etime(
                                      ctx->built_edarray, i),
                                  &esec, &eusec);
                printf("%s (%ld:%ld-%ld:%ld)\n", 
                       filename, ssec, susec, esec, eusec);
            }
        }
#endif
        ctx->total_sec = ___compute_edit_total_secs(
                             ctx->gvctx, ctx->built_edarray, built_len);
        ctx->edit_indx += built_len;
        ctx->edfiles_ctx = mpgedit_edit_files_init5(
                               ctx->built_edarray, ctx->out_filename,
                               ctx->edit_flags, &ctx->edit_stats, &sts);
        if (ctx->edfiles_ctx && sts == 0) {
            ctx->edfiles_ctx_editing = 
                mpgedit_edit_files(ctx->edfiles_ctx, &sts);
        }
        if (sts) {
            ctx->state = EDIT_IDLE_STATE_ERROR;
        }
        else if (!ctx->edfiles_ctx_editing) {
            ctx->state = EDIT_IDLE_STATE_LOOP;
            mpgedit_edit_files_free(ctx->edfiles_ctx);
        }
        else {
            ctx->state = EDIT_IDLE_STATE_EDIT;
            if (ctx->dialog) {
                gtk_widget_destroy(ctx->dialog);
            }
            ctx->dialog = ___make_editfile_progress_dialog(
                              ctx,
                              ctx->gvctx->main_window,
                              ctx->out_filename,
                              ctx->total_sec);
            gtk_widget_show_all(ctx->dialog);
        }
        break;

      case EDIT_IDLE_STATE_EDIT:
        if (ctx->quit) {
            ctx->state = EDIT_IDLE_STATE_DONE;
            break;
        }
        ctx->current_sec++;
        ctx->edfiles_ctx_editing = 
            mpgedit_edit_files(ctx->edfiles_ctx, &sts);
        percent = ((gdouble)ctx->current_sec) / ((double)ctx->total_sec);
        if (percent > 1.0) {
            percent = 1.0;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctx->progress), 
                                      percent);
        
        if (!ctx->edfiles_ctx_editing) {
            ctx->state = EDIT_IDLE_STATE_LOOP;
            mpgedit_edit_files_free(ctx->edfiles_ctx);
        }
        if (sts) {
            ctx->state = EDIT_IDLE_STATE_ERROR;
        }
        break;

      case EDIT_IDLE_STATE_DONE:
        rval = FALSE;
        if (ctx->dialog) {
            gtk_widget_destroy(ctx->dialog);
            ctx->dialog = NULL;
        }

        /* adam/TBD: free idle context memory here */
        break;

      case EDIT_IDLE_STATE_ERROR:
        if (ctx->dialog) {
            gtk_widget_destroy(ctx->dialog);
            ctx->dialog = NULL;
        }
        ctx->quit = TRUE;
        dialog = ___make_dialog_informational(ctx->gvctx->main_window,
                     FALSE,
                     ctx->out_filename,
                     "        ERROR: Edit output "
                     "file already exists       \n\n");
        if (dialog) {
            gtk_widget_show_all(dialog);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            ctx->dialog = NULL;
        }
        rval = FALSE;
        ctx->state = EDIT_IDLE_STATE_INIT;
        break;
    }

    return rval;
}


void ___load_editor_by_editspec(global_vars_ctx *gvctx,
                                editspec_t *edarray,
                                int edlen)
{
    int             i;
    int             j;
    edit_row_ctx    *edit_row;
    edit_row_ctx    *cursor = NULL;
    int             found;
    char            *item[4];
    char            duration[128];
    char            *filename;
    long            ssec;
    long            susec;
    long            esec;
    long            eusec;
    mpeg_time       dtime;

#ifdef _DEBUG
    printf("___load_editor_by_editspec: called\n");
#endif
    if (gvctx->idlectx->play_file) {
        free(gvctx->idlectx->play_file);
    }
    gvctx->idlectx->play_file  = strdup(mpgedit_editspec_get_file(edarray, 0));
    gtk_widget_hide(gvctx->editctx.clist);
    for (i=0; i<edlen; i++) {
        filename = mpgedit_editspec_get_file(edarray, i);
        mpeg_time_gettime(mpgedit_editspec_get_stime(edarray, i),
                          &ssec, &susec);
        mpeg_time_gettime(mpgedit_editspec_get_etime(edarray, i),
                          &esec, &eusec);
        if (esec == MP3_TIME_INFINITE) {
            esec = 0;
        }
        edit_row = ___row_make(filename, i,
                               ssec/60, ssec%60, susec/1000,
                               esec/60, esec%60, eusec/1000, gvctx);
        ___edit_row_list_append(edit_row, &gvctx->editctx);
        duration[0] = '\0';
        if (edit_row->file_name && *edit_row->file_name) {
            dtime = mpeg_time_compute_delta(ssec, susec, esec, eusec);
            sprintf(duration, "%ld:%02ld:%03ld", dtime.units/60, dtime.units%60, dtime.usec/1000);
        }
        item[0] = edit_row->file_name;
        item[1] = edit_row->stime_str;
        item[2] = edit_row->etime_str;
        item[3] = duration;
        gtk_clist_append(GTK_CLIST(gvctx->editctx.clist), item);
        if (!cursor) {
            cursor = edit_row;
        }
        /*
         * Don't index any file name already present in edit array
         */
        found = 0;
        for (j=0; j<i; j++) {
            if (!strcmp(mpgedit_editspec_get_file(edarray, j), 
                        mpgedit_editspec_get_file(edarray, i)))
            {
                found = 1;
            }
        }
        if (!found) {
            ___indexfile_init(NULL, NULL, edit_row, cb_row_clicked_1);
        }
    }
    gtk_widget_show(gvctx->editctx.clist);
    gvctx->editctx.cursor = cursor;
}


void ___load_editor_by_files(global_vars_ctx *gvctx, char **files, int len)
{
    int        i;
    editspec_t *edarray;

    edarray = mpgedit_editspec_init();
    if (!edarray) {
        return;
    }

    for (i=0; i<len; i++) {
        mpgedit_editspec_append(edarray, files[i], "0:0.0-0:0.0");
    }
    ___load_editor_by_editspec(gvctx, edarray, len);
}


void cb_load(GtkWidget *widget, gint response, gpointer data)
{
    global_vars_ctx *gvctx = (global_vars_ctx *) data;
    editspec_t      *edarray;
    int             edlen;
    GtkAdjustment   *adj;
    GtkWidget       *dialog;

    if (response == GTK_RESPONSE_OK) {
        edlen = ___read_abandoned_edits(gvctx->abandon_file, &edarray);
        if (edlen > 0) {
            if (gvctx->editctx.head && gvctx->editctx.head->next) {
                /*
                 * First line of editor is always the pad blank line, so
                 * dirty the editor when there are lines after this pad.
                 */
                gvctx->editctx.dirty = TRUE;
                gtk_widget_set_sensitive(gvctx->menu_save_session, TRUE);
            }
            ___load_editor_by_editspec(gvctx, edarray, edlen);
            
            gtk_clist_select_row(GTK_CLIST(gvctx->editctx.clist),
                                 gvctx->editctx.cursor->row_num, 0);
            if (gvctx->editctx.cursor->row_num > 0) {
                adj = gtk_scrolled_window_get_vadjustment(
                          GTK_SCROLLED_WINDOW(gvctx->editctx.scrolled_window));
                gtk_adjustment_set_value(adj, adj->upper);
            }
        }
        else {
           dialog = ___make_dialog_informational(
                        gvctx->main_window,
                        FALSE, gvctx->abandon_file, 
                        "        ERROR: Failed opening "
                        "saved session file        \n\n");
           gtk_widget_show_all(dialog);
           gtk_dialog_run(GTK_DIALOG(dialog));
           gtk_widget_destroy(dialog);
        }
    }
}



void cb_edit(GtkWidget *widget, gpointer data)
{
    global_vars_ctx   *gvctx = (global_vars_ctx *) data;
    int               playing = 0;
    editspec_t        *edarray;
    cb_edit_ctx       edctx;
    int               edarray_len;
    int               edit_flags = 0;
    char              *saved_edits_file;
    idle_editfile_ctx *edidle_ctx;
    GtkWidget         *dialog;

    if (!gvctx->editctx.head) {
        return;
    }
    edarray_len = gvctx->editctx.tail->row_num;

    edarray = mpgedit_editspec_init();
    if (!edarray) {
        return;
    }

    memset(&edctx, 0, sizeof(edctx));
    edctx.gvctx   = gvctx;
    edctx.len     = edarray_len;
    edctx.edarray = edarray;

    if (gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(gvctx->idlectx->player->button_play)))
    {
        playing |= 1;
    }
 
    if (playing) {
        mpgedit_play_close(gvctx->idlectx->playctx);
        gvctx->idlectx->playctx = NULL; 
    }

    ___edit_row_list_iterate(&gvctx->editctx, ___list_iterator_func, &edctx);

    /*
     * Overwrite of abandoned edits file has already been
     * accepted by query_abandon_ctx; see cb_edit_1().
     */
    saved_edits_file = gvctx->abandon_file;
    if (!___write_abandoned_edits(saved_edits_file, 1,
                                  edarray, edarray_len))
    {
        dialog = ___make_dialog_informational(gvctx->main_window,
                     FALSE,
                     saved_edits_file,
                     "        ERROR: Abandoned session "
                     "save file already exists        \n\n");
        gtk_widget_show_all(dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        gvctx->edit_action_abandon_save = FALSE;
        return;
    }
    gvctx->editctx.dirty = FALSE;
    gtk_widget_set_sensitive(gvctx->menu_save_session, FALSE);

    /* 
     * Action was only to save abandon edits when set
     */
    if (gvctx->edit_action_abandon_save) {
        gvctx->edit_action_abandon_save = FALSE;
        return;
    }

    /*
     * All of the file split editing is performed in the 
     * idle proc function cb_edit_idle_editfile().
     */
    edidle_ctx = calloc(1, sizeof(*edidle_ctx));
    if (edidle_ctx) {
        edidle_ctx->gvctx       = gvctx;
        edidle_ctx->edfiles_ctx = NULL;
        edidle_ctx->edfiles_ctx_editing = 0;
        edidle_ctx->edarray     = edarray;
        edidle_ctx->edarray_len = edarray_len;
        edidle_ctx->edit_flags  = edit_flags;
        gtk_idle_add(cb_edit_idle_editfile, edidle_ctx);
    }
}



/*
 * This layer is needed because we are called by ___indexfile_init,
 * which passes as 3 arguments.  cb_edit() only takes widget, and user
 * data, so we must "eat" the response argument
 */
void cb_edit_3(GtkWidget *widget, GdkEvent *dummy, gpointer data)
{
    edit_row_ctx *er = (edit_row_ctx *) data;

    cb_edit(widget, er->ctx);
}


void cb_edit_1_rmd(gpointer data)
{
    global_vars_ctx *gvctx = (global_vars_ctx *) data;

#ifdef _DEBUG
    printf("cb_edit_1_rmd: called\n");
#endif
    cb_edit_1a(gvctx->row_modified_ctx.widget, data);
}


void cb_edit_1a(GtkWidget *widget, gpointer data)
{
    global_vars_ctx   *gvctx = (global_vars_ctx *) data;
    int               len;
    editspec_t        *edarray;
    edit_row_ctx      *er;

#ifdef _DEBUG
    printf("cb_edit_1a: called");
#endif
    /*
     * Test existence of abandoned session file, and display warning
     * dialog when already exists.
     */
    len = ___read_abandoned_edits(gvctx->abandon_file, &edarray);
    if (len > 0 && gvctx->editctx.dirty) {
        free(edarray);
        gvctx->query_abandon_ctx = 
            ___make_dialog_outputfile_title(
                gvctx,
                "Save Session",
                "\n    Abandoned file already exists, overwrite?    ",
                FALSE,
                &gvctx->abandon_file,
                NULL);
        g_signal_connect(GTK_OBJECT (gvctx->query_abandon_ctx->dialog),
                          "response",
                          G_CALLBACK(cb_dialog_outputfile),
                          (gpointer) gvctx->query_abandon_ctx);
        g_signal_connect(GTK_OBJECT (gvctx->query_abandon_ctx->dialog),
                          "response",
                          G_CALLBACK(cb_edit_2),
                          (gpointer) gvctx);
    }
    else {
        er = (edit_row_ctx *) calloc(1, sizeof(edit_row_ctx));
        if (er) {
            er->ctx = gvctx;
            ___indexfile_init(widget, NULL, er, cb_edit_3);
        }
    }
}


void cb_edit_1(GtkWidget *widget, gpointer data)
{
    global_vars_ctx   *gvctx = (global_vars_ctx *) data;

#ifdef _DEBUG
    printf("cb_edit_1: called\n");
#endif
    /*
     * Do nothing when action is save session, but buffer is not dirty.
     */
    if (gvctx->edit_action_abandon_save && !gvctx->editctx.dirty) {
        gvctx->edit_action_abandon_save = FALSE;
        return;
    }

    /*
     * Prompt to save changes to current row
     */
    if (gvctx->idlectx->dirty) {
        gvctx->row_modified_ctx.proc   = cb_edit_1_rmd;
        gvctx->row_modified_ctx.data   = gvctx;
        gvctx->row_modified_ctx.widget = widget;
        ___row_modified_dialog(gvctx);
        return;
    }

    cb_edit_1a(widget, gvctx);
}


void cb_edit_2(GtkWidget *widget, gint response,  gpointer data)
{
    global_vars_ctx   *gvctx = (global_vars_ctx *) data;

    if (response == GTK_RESPONSE_OK) {
        ___put_default_abandoned_filename(gvctx->abandon_file);
        cb_edit(widget, data);
    }
}


void cb_destroy(GtkWidget *widget, gpointer data)
{
    global_vars_ctx *gvctx = (global_vars_ctx *) data;
    cb_edit_ctx     edctx;
    gint            rsp;

    if (!gvctx->editctx.dirty) {
        return;
    }

    memset(&edctx, 0, sizeof(edctx));
    edctx.gvctx   = gvctx;
    edctx.edarray = mpgedit_editspec_init();
    if (!edctx.edarray) {
        return;
    }

    edctx.len = gvctx->editctx.tail->row_num;
    ___edit_row_list_iterate(&gvctx->editctx, ___list_iterator_func, &edctx);
    rsp = ___make_dialog_informational_run_title(gvctx, TRUE,
               "Save Session?",
               "\n\n"
               "          Last chance to save modified session!        \n"
               "          Do you want to save edits?        \n\n");
                
    if (rsp == GTK_RESPONSE_OK) {
        if (!___write_abandoned_edits(gvctx->abandon_file, 1,
                                      edctx.edarray,
                                      gvctx->editctx.tail->row_num))
        {
            printf("failed writing edits\n");
        }
    }
}


gint cb_quit(GtkWidget *widget, gpointer data)
{
    idle_ctx  *idlectx = (idle_ctx *) data;
    gint      response;
    int       playing = gtk_toggle_button_get_active(
                            GTK_TOGGLE_BUTTON(idlectx->player->button_play));

    if (playing) {
        mpgedit_play_close(idlectx->playctx);
        idlectx->playctx = NULL; 
    }

    if (idlectx->gvctx->editctx.dirty) {
        response = ___editor_dirty_dialog_run(idlectx->gvctx);
        if (response != GTK_RESPONSE_OK) {
            return FALSE;
        }
    }

    /* Write config values, as last directory opened probably changed */
    write_config_file(CONFIG_FILE_NAME, idlectx->gvctx);

    gtk_main_quit();
    return TRUE;
}


void cb_pause_button(GtkWidget *widget, gpointer data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    int playing;
    int paused;

    paused = gtk_toggle_button_get_active(
                 GTK_TOGGLE_BUTTON(idlectx->player->button_pause));
    playing = gtk_toggle_button_get_active(
                  GTK_TOGGLE_BUTTON(idlectx->player->button_play));
    if (playing) {
        if (paused) {
            idlectx->pause_sec  = mpgedit_play_current_sec(idlectx->playctx);
            idlectx->pause_usec = mpgedit_play_current_msec(idlectx->playctx);
            mpgedit_play_reset_audio(idlectx->playctx);
            mpgedit_play_close(idlectx->playctx);
            idlectx->playctx = NULL;
            ___init_volume(idlectx->gvctx->volumectx, NULL);
            gtk_idle_remove(idlectx->idle_tag);
        }
        else {
            idlectx->playctx = mpgedit_play_init(idlectx->play_file, 0);
            if (!idlectx->playctx) {
                ___make_cant_open_sound(idlectx->gvctx);
               return;
            }
            ___init_volume(idlectx->gvctx->volumectx, idlectx->playctx);

            mpgedit_play_set_status_callback(idlectx->playctx,
                                             cb_label_set_text, 
                                             idlectx);
            mpgedit_play_seek_time(idlectx->playctx,
                                   idlectx->pause_sec, idlectx->pause_usec);
            idlectx->idle_tag = gtk_timeout_add(PLAY_IDLE_MSEC, 
                                              idle_play, idlectx);
        }
    }
    else {
        if (paused) {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(idlectx->player->button_pause), FALSE);
        }
    }
}


void cb_stop_button(GtkWidget *widget, gpointer data)
{
    idle_ctx *idlectx = (idle_ctx *) data;

    /* Must push button out because it is a toggle button */
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(idlectx->player->button_stop), FALSE);

    if (gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(idlectx->player->button_playto))) 
    {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_playto), FALSE);
    }
    else if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_play)))
    {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
    }
}


void cb_play_button(GtkWidget *widget, gpointer data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    global_vars_ctx *gvctx = idlectx->gvctx;
    mpgedit_pcmview_ctx *pcmctx = idlectx->gvctx->pcmviewctx;
    int playing;
    long seek_sec;

    /*
     * Quit if playto is running
     */
    playing = gtk_toggle_button_get_active(
                  GTK_TOGGLE_BUTTON(idlectx->player->button_playto));
    if (playing) {
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
        return;
    }

    playing = gtk_toggle_button_get_active(
                  GTK_TOGGLE_BUTTON(idlectx->player->button_play));
    if (playing) {
        /*
         * Transitioned from stopped -> playing
         */
        if (!idlectx->play_file ||
            ___test_is_valid_file_error_dialog(idlectx->play_file,
                                               gvctx))
        {
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
            return;
        }
        idlectx->playctx = mpgedit_play_init(idlectx->play_file, 0);
        ___init_volume(gvctx->volumectx, idlectx->playctx);

        if (!idlectx->playctx) {
            ___make_cant_open_sound(gvctx);
            /*
             * adam/TBD: Do something more exciting when this fails
             */
            gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(idlectx->player->button_play), FALSE);
            return;
        }
        idlectx->save_stime_sec = idlectx->stime_sec;
        idlectx->save_stime_usec = idlectx->stime_usec;
        ___add_offset_time(0, idlectx->stime_sec, idlectx->stime_usec,
                           mpeg_time_getsec(&idlectx->gvctx->offset_time),
                           mpeg_time_getusec(&idlectx->gvctx->offset_time), 
                           &idlectx->stime_sec, &idlectx->stime_usec);
        mpgedit_play_seek_time(idlectx->playctx, 
                               idlectx->stime_sec, idlectx->stime_usec);
#ifdef _DEBUG
        printf("cb_play_button: "
               "stime=%d:%d | elapsed=%d:%d size=%ld sec=%ld usec=%ld\n",
               idlectx->stime_sec, idlectx->stime_usec,
               idlectx->elapsed_sec, idlectx->elapsed_usec,
               mpgedit_play_total_size(idlectx->playctx),
               mpgedit_play_total_sec(idlectx->playctx),
               mpgedit_play_total_msec(idlectx->playctx));
#endif
        /*
         * scroll window left when first sec is off the left margin
         */
            
        if (idlectx->stime_sec < pcmctx->pcm_secfirst) {
            seek_sec = idlectx->stime_sec - 
                           (pcmctx->pcm_seclast - pcmctx->pcm_secfirst) +
                            PLAYTO_BACKUP_SEC;
            pcmctx->seek_sec(pcmctx, seek_sec);
            ___pcmview_draw_cursor_sec(idlectx->gvctx,
                                       idlectx->stime_sec,
                                       idlectx->stime_usec, 100);
        }

        idlectx->idle_tag = gtk_timeout_add(PLAY_IDLE_MSEC, idle_play, idlectx);
        mpgedit_play_set_status_callback(idlectx->playctx,
                                         cb_label_set_text, 
                                         idlectx);
    }
    else {
        /*
         * Transitioned from playing -> stopped
         */
        idlectx->stime_sec    = idlectx->save_stime_sec;
        idlectx->stime_usec   = idlectx->save_stime_usec;

        idlectx->elapsed_sec  = idlectx->stime_sec;
        idlectx->elapsed_usec = idlectx->stime_usec;
        idlectx->gvctx->ledlen->settime(idlectx->gvctx->ledlen,
                                        idlectx->stime_sec,
                                        idlectx->stime_usec);
        mpgedit_play_reset_audio(idlectx->playctx);
        mpgedit_play_close(idlectx->playctx);
        idlectx->playctx = NULL; 
        ___init_volume(gvctx->volumectx, NULL);
        if (idlectx->idle_tag) {
            gtk_idle_remove(idlectx->idle_tag);
        }
        gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(idlectx->player->button_pause), FALSE);

        ___spin_button_set_value(&idlectx->player->sbctx,
                                 idlectx->stime_sec,
                                 idlectx->stime_usec);
    }
    return;
}


void cb_window_button_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    global_vars_ctx *gvctx       = (global_vars_ctx *) data;
    GdkEventButton *event_button = (GdkEventButton *) event;

#ifdef _DEBUG
    printf("cb_window_button_press: called widget=%p\n", widget);
#endif
    if (event->type == GDK_BUTTON_PRESS) {
#ifdef _DEBUG
        printf("cb_window_button_press: event_button: %d\n",
               event_button->button);
#endif
        if (event_button->button == 3) {
            gtk_menu_popup (GTK_MENU (gvctx->main_menu), NULL, NULL, NULL,
                            NULL, event_button->button, event_button->time);

        }
    }
}


void cb_dialog_abandonfile_save(GtkWidget *dialog,
                                gint response,
                                gpointer data)
{
    outfile_dialog_ctx *ctx = (outfile_dialog_ctx *) data;

#ifdef _DEBUG
    printf("cb_dialog_abandonfile_save: called %s\n", *ctx->file);
#endif
    if (response == GTK_RESPONSE_OK) {
        ___put_default_abandoned_filename(*ctx->file);
    }
}



void cb_dialog_outputfile(GtkWidget *dialog, gint response, gpointer data)
{
    outfile_dialog_ctx *ctx = (outfile_dialog_ctx *) data;
    gint fs_response;
    GtkWidget *fs;

#ifdef _DEBUG
    printf("cb_dialog_outputfile: Response code = %d\n", response);
#endif
    if (response == GTK_RESPONSE_YES) {
        fs = gtk_file_selection_new("Output file select");
        gtk_widget_show(fs);

        /* Hide delete and rename buttons */
        gtk_widget_hide(GTK_FILE_SELECTION(fs)->fileop_del_file);
        gtk_widget_hide(GTK_FILE_SELECTION(fs)->fileop_ren_file);

        fs_response = gtk_dialog_run(GTK_DIALOG(fs));
        if (fs_response == GTK_RESPONSE_OK) {
            gtk_entry_set_text(GTK_ENTRY(ctx->entry),
                strtrim(gtk_file_selection_get_filename(
                        GTK_FILE_SELECTION(fs))));
            response = GTK_RESPONSE_OK;
        }
        gtk_widget_destroy(fs);
        return;
    }
    gtk_widget_hide_all(ctx->dialog);
    if (response == GTK_RESPONSE_OK) {
        ctx->action = ctx->radio_selection;
        if (ctx->action == OUTPUT_FILE_APPEND_DEFAULT) {
            ctx->action = OUTPUT_FILE_APPEND_SPLIT;
        }

        if (*ctx->file) {
            free(*ctx->file);
        }
        *ctx->file = strtrim(gtk_entry_get_text(GTK_ENTRY(ctx->entry)));
        if (ctx->radio_action) {
            *ctx->radio_action = ctx->action;
        }
    }
    else if (response == GTK_RESPONSE_REJECT) {
        gtk_entry_set_text(GTK_ENTRY(ctx->entry), *ctx->file);
    }
#ifdef _DEBUG
    printf("cb_dialog_outputfile: output_file=%s\n", ctx->gvctx->output_file);
    printf("cb_dialog_outputfile: abandon_file=%s\n", ctx->gvctx->abandon_file);
#endif
}


void cb_outfile_radio(GtkWidget *widget, gpointer data)
{
    outfile_radio_ctx *rbctx = (outfile_radio_ctx *) data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        *rbctx->radio_selection = rbctx->button;
    }
}


outfile_dialog_ctx *___make_dialog_outputfile_title(global_vars_ctx *gvctx,
                                              char *title,
                                              char *name_label,
                                              int  show_radio,
                                              char **output_name,
                                              int *radio_action)
{
    GtkWidget *dialog;
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *radio;
    GtkWidget *radio_join = NULL;
    GtkWidget *entry;
    outfile_radio_ctx *rbctx;
    outfile_dialog_ctx *ctx;

    ctx = (outfile_dialog_ctx *) calloc(1, sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    dialog = gtk_dialog_new_with_buttons(title,
                                         GTK_WINDOW(gvctx->main_window),
                                         GTK_DIALOG_MODAL, 
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                         GTK_STOCK_OPEN,   GTK_RESPONSE_YES,
                                         GTK_STOCK_OK,     GTK_RESPONSE_OK,
                                         NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    label = gtk_label_new(name_label);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       label, FALSE, FALSE, 0);


    /*
     * Text entry 
     */
    entry = gtk_entry_new();
    gtk_widget_show(entry);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, TRUE, TRUE, 0);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);


    if (show_radio) {
        /*
         * Radio buttons
         */
        box = gtk_vbox_new(FALSE, 0);
        radio = gtk_radio_button_new_with_label (NULL, "Join");
        radio_join = radio;
        gtk_box_pack_start (GTK_BOX (box), radio, TRUE, TRUE, 0);
            gtk_widget_show (radio);
        rbctx = (outfile_radio_ctx *) calloc(1, sizeof(*rbctx));
        rbctx->button = OUTPUT_FILE_APPEND_JOIN;
        rbctx->radio_selection = &ctx->radio_selection;
        rbctx->gvctx = gvctx;
        g_signal_connect(G_OBJECT(radio), "toggled",
                         G_CALLBACK(cb_outfile_radio), rbctx);
        radio = gtk_radio_button_new_with_label_from_widget (
                     GTK_RADIO_BUTTON (radio), "Split");
        gtk_box_pack_start (GTK_BOX (box), radio, TRUE, TRUE, 0);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
        gtk_widget_show (radio);
        rbctx = (outfile_radio_ctx *) calloc(1, sizeof(*rbctx));
        rbctx->button = OUTPUT_FILE_APPEND_SPLIT;
        rbctx->radio_selection = &ctx->radio_selection;
        rbctx->gvctx = gvctx;
        g_signal_connect(G_OBJECT(radio), "toggled",
                         G_CALLBACK(cb_outfile_radio), rbctx);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                           box, TRUE, TRUE, 0);
    }

    ctx->gvctx  = gvctx;
    ctx->file   = output_name;
    ctx->radio_action = radio_action;
    ctx->dialog = dialog;
    ctx->entry  = entry;

    gtk_entry_set_text(GTK_ENTRY(ctx->entry), *output_name ? *output_name : "");
    if (radio_action && *radio_action == OUTPUT_FILE_APPEND_JOIN) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_join), TRUE);
    }
    gtk_widget_show_all(dialog);

    return ctx;
}


outfile_dialog_ctx *___make_dialog_outputfile(global_vars_ctx *gvctx,
                                              char *name_label,
                                              int  show_radio,
                                              char **output_name,
                                              int *radio_action)
{
    return ___make_dialog_outputfile_title(gvctx, name_label, name_label,
                                           show_radio, output_name,
                                           radio_action);
}


/*
 * Populate other default values in the context
 */
void ___set_editor_defaults(global_vars_ctx *gvctx)
{
    if (gvctx->idlectx->play_file) {
        free(gvctx->idlectx->play_file);
    }
    gvctx->idlectx->play_file  = NULL;
    gvctx->idlectx->dirty      = FALSE;

    /*
     * Populate other default values in the context
     */
    if (gvctx->output_file) {
       free(gvctx->output_file);
    }
    gvctx->output_file = strdup("");

    if (gvctx->abandon_file) {
        free(gvctx->abandon_file);
    }
    gvctx->abandon_file  = ___get_default_abandoned_filename(NULL);
    gvctx->output_action = OUTPUT_FILE_APPEND_DEFAULT;
}


void init_rc_file(char *file)
{
    char            **def_files;
    char            *cp;
    char            *rc_path = NULL;

    /*
     * Get list of Gtk rc files.  Assume last in list is
     * path to rc file in home directory.  When found, use this list
     * to build path to .xmpgedit-gtkrc file in home directory.
     */
    def_files = gtk_rc_get_default_files();
    if (def_files) {
        while (*(def_files+1)) {
            def_files++;
        }
        cp = strrchr(def_files[0], '/');
        if (cp) {
            rc_path = calloc(1, (cp-def_files[0]) +
                                sizeof(GTK_RC_FILE) + 1);
        }
        if (rc_path) {
            strncat(rc_path, def_files[0], cp-def_files[0]);
            strcat(rc_path, GTK_RC_FILE);
        }
        gtk_rc_add_default_file(rc_path);
        free(rc_path);
    }
}



void cb_edittime(gpointer data, 
                 long secbegin, long msecbegin,
                 long secend, long msecend)
{

    mpgedit_pcmview_ctx *pcmctx = (mpgedit_pcmview_ctx *) data;
    global_vars_ctx *gvctx = pcmctx->get_data(pcmctx);
    idle_ctx        *idlectx = gvctx->idlectx;

    int             changed;

#ifdef _DEBUG
    printf("cb_edittime: <<<<< %ld:%02ld.%03ld - %ld:%02ld.%03ld >>>>>\n",
           secbegin/60, secbegin%60, msecbegin, secend/60, secend%60, msecend);
#endif

    if (gvctx->editctx.cursor->row_num == 0) {
        return;
    }

    changed = ___row_init(gvctx->editctx.cursor, NULL, 
                          secbegin/60, secbegin%60, msecbegin,
                          secend/60, secend%60, msecend);
    idlectx->stime_sec  = secbegin;
    idlectx->stime_usec = msecbegin;

    idlectx->dirty = idlectx->dirty_saved = FALSE;
    /*
     * Nothing has changed, so just quit this callback
     */
    if (!changed) {
        return;
    }
    ___row_update(&gvctx->editctx, gvctx->editctx.clist);
    gvctx->editctx.dirty = TRUE;
    gtk_widget_set_sensitive(gvctx->menu_save_session, gvctx->editctx.dirty);
}



void cb_button_press_event_block(GtkWidget *widget, gpointer data)
{
    mpgedit_pcmview_ctx *pcmctx = (mpgedit_pcmview_ctx *) data;
    global_vars_ctx *gvctx = pcmctx->get_data(pcmctx);
    spin_button_ctx_t *sbctx = &gvctx->idlectx->player->sbctx;

#ifdef _DEBUG
    printf("cb_button_press_event_block: called\n");
#endif
    g_signal_handler_block(sbctx->spin_msec, sbctx->hmsec);
    g_signal_handler_block(sbctx->spin_sec,  sbctx->hsec);
    g_signal_handler_block(sbctx->spin_min,  sbctx->hmin);
    g_signal_handler_block(sbctx->scale,     sbctx->hscale);
    set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, TRUE);
}


void cb_button_press_event_unblock(GtkWidget *widget, gpointer data)
{
    mpgedit_pcmview_ctx *pcmctx = (mpgedit_pcmview_ctx *) data;
    global_vars_ctx *gvctx = pcmctx->get_data(pcmctx);
    spin_button_ctx_t *sbctx = &gvctx->idlectx->player->sbctx;

#ifdef _DEBUG
    printf("cb_button_press_event_unblock: called\n");
#endif
    g_signal_handler_unblock(sbctx->spin_msec, sbctx->hmsec);
    g_signal_handler_unblock(sbctx->spin_sec,  sbctx->hsec);
    g_signal_handler_unblock(sbctx->spin_min,  sbctx->hmin);
    g_signal_handler_unblock(sbctx->scale,     sbctx->hscale);
    set_block_pcmview_flag(&gvctx->idlectx->player->sbctx, FALSE);
}


void cb_pcmview_values_block(GtkWidget *widget, gpointer data)
{
    mpgedit_pcmview_ctx *pcmctx = (mpgedit_pcmview_ctx *) data;
    global_vars_ctx *gvctx = pcmctx->get_data(pcmctx);
    spin_button_ctx_t *sbctx = &gvctx->idlectx->player->sbctx;

#ifdef _DEBUG
    printf("cb_pcmview_values_block: called\n");
#endif
    g_signal_handler_block(sbctx->spin_msec, sbctx->hmsec);
    g_signal_handler_block(sbctx->spin_sec,  sbctx->hsec);
    g_signal_handler_block(sbctx->spin_min,  sbctx->hmin);
    g_signal_handler_block(sbctx->scale,     sbctx->hscale);
    g_signal_handler_block(sbctx->adj_scale, sbctx->hadj_scale);
}


void cb_pcmview_values_unblock(GtkWidget *widget, gpointer data)
{
    mpgedit_pcmview_ctx *pcmctx = (mpgedit_pcmview_ctx *) data;
    global_vars_ctx *gvctx = pcmctx->get_data(pcmctx);
    spin_button_ctx_t *sbctx = &gvctx->idlectx->player->sbctx;

#ifdef _DEBUG
    printf("cb_pcmview_values_unblock: called\n");
#endif
    g_signal_handler_unblock(sbctx->spin_msec, sbctx->hmsec);
    g_signal_handler_unblock(sbctx->spin_sec,  sbctx->hsec);
    g_signal_handler_unblock(sbctx->spin_min,  sbctx->hmin);
    g_signal_handler_unblock(sbctx->scale,     sbctx->hscale);
    g_signal_handler_unblock(sbctx->adj_scale, sbctx->hadj_scale);
}


void cb_pcmview_values_set(GtkWidget *widget, gpointer data)
{
#ifdef _DEBUG
    printf("cb_pcmview_values_set called\n");
#endif
    ___pcmview_set_values(data);
}


#ifdef LEDPCM_DISPLAY
void cb_pcmview_motion(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    global_vars_ctx *gvctx = data;

    gvctx->ledpcm->setinteger(gvctx->ledpcm,
                              gvctx->pcmviewctx->pcm_cursorvalue);
}
#endif


GtkWidget * __make_widget_file_open(global_vars_ctx *gvctx)
{
    GtkWidget *box_fopen;
    GtkWidget *button;
    GtkWidget *arrow;
    GtkWidget *entry;
    GtkWidget *label;
    char      *play_file = "";

    /*
     * Open button
     */
    box_fopen = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_fopen);

    /* A label */
    label = gtk_label_new("File: ");
    gtk_widget_show(label);

    /* A text entry field */
    entry = gtk_entry_new();
    gtk_tooltips_set_tip(gvctx->tooltips_entry, GTK_WIDGET(entry), 
                         play_file, NULL);
    gtk_entry_set_text(GTK_ENTRY(entry), play_file);
    gtk_editable_set_position(GTK_EDITABLE(entry), strlen(play_file));
    gtk_entry_set_max_length(GTK_ENTRY(entry), PATH_MAX);
    gtk_widget_show(entry);
    g_signal_connect(G_OBJECT(entry), "activate",
                     G_CALLBACK(cb_entry), gvctx);
    gvctx->entry = entry;

    /* An arrow button */
    button = gtk_button_new();
    gtk_widget_show(button);
    gvctx->open_file_toggle  = button;
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_open_file), gvctx);
    arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_OUT);
    gtk_widget_show(arrow);
    gtk_container_add(GTK_CONTAINER(button), arrow);

    gtk_box_pack_start(GTK_BOX(box_fopen), label,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_fopen), entry,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_fopen), button, FALSE, FALSE, 0);

    return box_fopen;
}


mpgedit_pcmview_ctx *___make_pcmview_control(global_vars_ctx *gvctx)
{
    mpgedit_pcmview_ctx *pcmctx;
    record_button_ctx_t *recbtn_ctx;
    GtkWidget           *button;
    GtkWidget           *imgbox;
    GtkWidget           *separator;
    GtkWidget           *pcmhbox;
    GtkWidget           *pcmvbox;
    GtkWidget           *arrow;
    GtkWidget           *recbuttons_box;
#ifdef LEDPCM_DISPLAY
    GtkWidget           *ledwidget;
    GtkWidget           *ledframe;
#endif
    long                offset_sec;
    long                offset_msec;

    /* PCM waveform viewer */
    pcmctx = mpgedit_pcmview_new();
    pcmctx->set_click_callback(pcmctx,  cb_pcmview);
    pcmctx->set_click_callback_pre(pcmctx,  cb_button_press_event_block);
    pcmctx->set_click_callback_post(pcmctx, cb_button_press_event_unblock);
    pcmctx->set_edittime_callback(pcmctx,   cb_edittime);
#ifdef LEDPCM_DISPLAY
    pcmctx->set_motionnotify_callback(pcmctx, cb_pcmview_motion, gvctx);
#endif
    pcmctx->set_silence_db(pcmctx, SILENCE_30DB);
    pcmctx->button_mask = 0x1;

    mpeg_time_gettime(&gvctx->offset_time, &offset_sec, &offset_msec);
    pcmctx->set_offset(pcmctx, offset_sec, offset_msec);

    pcmvbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(pcmvbox);

    button = gtk_button_new();
    gtk_widget_show(button);
    gtk_container_set_border_width(GTK_CONTAINER(button), 0);

    imgbox = xpm_pixmap_box(gtk_settings_get_default(), IMAGE_CLOSE);
    gtk_container_set_border_width(GTK_CONTAINER(imgbox), 0);
    gtk_widget_show(imgbox);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_box_pack_end(GTK_BOX(pcmctx->topbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_hide_viewfile), gvctx);
    
    gtk_box_pack_start(GTK_BOX(pcmctx->bottombox), pcmvbox, TRUE, TRUE, 0);
    
    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(pcmvbox), separator, FALSE, FALSE, 5);

    pcmhbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(pcmhbox);
    gtk_box_pack_start(GTK_BOX(pcmvbox), pcmhbox, FALSE, FALSE, 0);

    /* Scroll waveform left */
    button = gtk_button_new();
    gtk_widget_show(button);
    arrow = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_OUT);
    gtk_widget_show(arrow);
    gtk_container_add(GTK_CONTAINER(button), arrow);
    gtk_tooltips_set_tip(gvctx->tooltips, GTK_WIDGET(button),
                         TIP_PCMVIEW_LEFT, NULL);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_pcmview_values_block),   pcmctx);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(pcmctx->previous_values),   pcmctx);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_pcmview_values_set),     pcmctx);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_pcmview_values_unblock), pcmctx);

    gtk_box_pack_start(GTK_BOX(pcmhbox), button,
                       FALSE, FALSE, 0);
  
    /* Scroll waveform right */
    button = gtk_button_new();
    gtk_widget_show(button);
    arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
    gtk_widget_show(arrow);
    gtk_container_add(GTK_CONTAINER(button), arrow);
    gtk_tooltips_set_tip(gvctx->tooltips, GTK_WIDGET(button),
                         TIP_PCMVIEW_RIGHT, NULL);

    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_pcmview_values_block),   pcmctx);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(pcmctx->next_values),       pcmctx);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_pcmview_values_set),     pcmctx);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_pcmview_values_unblock), pcmctx);

    gtk_box_pack_start(GTK_BOX(pcmhbox), button,
                       FALSE, FALSE, 0);

    recbuttons_box = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(recbuttons_box);
    gtk_box_pack_start(GTK_BOX(pcmhbox), recbuttons_box,
                       FALSE, FALSE, 6);

    /* Start record button */
    button = gtk_button_new();
    gtk_widget_show(button);
    imgbox = xpm_pixmap_box(gtk_settings_get_default(), IMAGE_RECORD_START);
    gtk_widget_show(imgbox);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_box_pack_start(GTK_BOX(recbuttons_box), button,
                       FALSE, FALSE, 0);
    recbtn_ctx = calloc(1, sizeof(record_button_ctx_t));
    if (recbtn_ctx) {
        recbtn_ctx->start = TRUE;
        recbtn_ctx->gvctx = gvctx;
    }
    g_signal_connect(G_OBJECT(button), "clicked", 
                     G_CALLBACK(cb_record_time), recbtn_ctx);
    gtk_tooltips_set_tip(gvctx->tooltips, GTK_WIDGET(button),
                         TIP_RECORD_START, NULL);
   
    /* Stop record button */
    button = gtk_button_new();
    gtk_widget_show(button);
    imgbox = xpm_pixmap_box(gtk_settings_get_default(), IMAGE_RECORD_STOP);
    gtk_widget_show(imgbox);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_box_pack_start(GTK_BOX(recbuttons_box), button,
                       FALSE, FALSE, 0);
    recbtn_ctx = calloc(1, sizeof(record_button_ctx_t));
    if (recbtn_ctx) {
        recbtn_ctx->start = FALSE;
        recbtn_ctx->gvctx = gvctx;
    }
    g_signal_connect(G_OBJECT(button), "clicked", 
                     G_CALLBACK(cb_record_time), recbtn_ctx);
    gtk_tooltips_set_tip(gvctx->tooltips, GTK_WIDGET(button),
                         TIP_RECORD_STOP, NULL);
   

    /* Chain current edit line */
    button = gtk_button_new();
    gtk_widget_show(button);
    arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
    gtk_widget_show(arrow);
    gtk_container_add(GTK_CONTAINER(button), arrow);
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(cb_menuitem_chain), gvctx);
    gtk_box_pack_start(GTK_BOX(pcmhbox), button,
                       FALSE, FALSE, 0);
    gtk_tooltips_set_tip(gvctx->tooltips, GTK_WIDGET(button),
                         TIP_PCMVIEW_SAVE, NULL);

#ifdef LEDPCM_DISPLAY
    gvctx->ledpcm = mpgedit_ledtime_new();
    ledwidget = gvctx->ledpcm->integer_init(gvctx->ledpcm, 0);
    gtk_widget_show(ledwidget);
    ledframe = gtk_frame_new("PCM Pick Value");
    gtk_widget_show(ledframe);
    gtk_container_add(GTK_CONTAINER(ledframe), ledwidget);
    gtk_box_pack_start(GTK_BOX(pcmhbox), ledframe, FALSE, FALSE, 5);
#endif

    return pcmctx;
}


void cb_ledtime(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    ledtime_cbctx_t *ctx = (ledtime_cbctx_t *) data;
    int format;

    format = ctx->widget->getformat(ctx->widget);
    if (format == LEDTIME_FORMAT_CANONICAL) {
        ctx->widget->secondtime_init(ctx->widget, 0, 0);
    }
    else if (format == LEDTIME_FORMAT_MINSEC) {
        ctx->widget->canontime_init(ctx->widget, 0, 0);
    }
    else if (format == LEDTIME_FORMAT_SECONDS) {
        ctx->widget->minsectime_init(ctx->widget, 0, 0);
    }
    else {
        ctx->widget->minsectime_init(ctx->widget, 0, 0);
    }
}


void usage(char *argv[])
{
    fprintf(stderr, "usage: %s %s %s\n",
        argv[0],
        "[-c] [-h] [-O offsec.offmsec] [-e [start[-[end]]]] [-f inputfile] |",
        "inputfile1 inputfile2");
    exit(1);
}


xmpgedit_cmd_args_t *_xmpgedit_parse_cmdline(int *argc, char *argv[])
{
    xmpgedit_cmd_args_t *retargs = NULL;
    int c;
    int sts = 0;
    int cnt = 0;
    char *cp;

    retargs = (xmpgedit_cmd_args_t *) calloc(1, sizeof(xmpgedit_cmd_args_t));
    if (!retargs) {
        return NULL;
    }
    
    while ((c = getopt(*argc, argv, "ce:f:hO:")) != -1) {
      switch (c) {
        case 'e':
          cnt++;
          if (!retargs->editspec) {
              retargs->editspec = mpgedit_editspec_init();
              if (!retargs) {
                  sts = 1;
                  goto clean_exit;
              }
          }
          mpgedit_editspec_append(retargs->editspec, NULL, optarg);
          break;

        case 'f':
          cnt++;
          if (!retargs->editspec) {
              retargs->editspec = mpgedit_editspec_init();
              if (!retargs) {
                  sts = 1;
                  goto clean_exit;
              }
          }
          mpgedit_editspec_append(retargs->editspec, optarg, NULL);
          break;

        case 'h':
          usage(argv);
          break;

        case 'O':
          cnt++;
          cp = optarg;
          retargs->offset = strdup(cp);
          retargs->offset_sec = strtol(cp, &cp, 10);
          if (cp && (*cp == ':' || *cp == '.')) {
              cnt++;
              cp++;
              retargs->offset_msec = strtol(cp, &cp, 10);
          }
          break;
        

        default:
          break;
      }
    }


    /*
     * Test for last argument. Use as filename when present.
     */
    if (optind < *argc) {
        if (!retargs->editspec) {
            retargs->editspec = mpgedit_editspec_init();
            if (!retargs) {
                sts = 1;
                goto clean_exit;
            }
        }
        mpgedit_editspec_append(retargs->editspec, argv[optind], NULL);
    }

    /* Reset command line length to number of options parsed off */
    if (cnt > 0) {
        *argc -= optind;
    }

clean_exit:
    if (cnt == 0) {
        sts = 1;
    }
    if (sts) {
        if (retargs->editspec) {
            mpgedit_editspec_free(retargs->editspec);
        }
        free(retargs);
        retargs = NULL;
    }
    return retargs;
}


int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *label;
    GtkWidget *label_trt;
    GtkWidget *label_offset;
    GtkWidget *box;
    GtkWidget *mainvbox;
    GtkWidget *mainhbox;
    GtkWidget *control_hbox;
    GtkWidget *control_vbox;
    GtkWidget *leftboxspacer;
    GtkWidget *rightboxspacer;
    GtkWidget *bottomboxspacer;
    GtkWidget *box_elapsed;
    GtkWidget *box_offset;
    GtkWidget *box_trt;
    GtkWidget *box_entry;
    GtkWidget *box_fopen;
    GtkWidget *vbox_playback;
    GtkWidget *hbox_playback;
    GtkWidget *frame_playback;
    record_button_ctx_t *recbtn_ctx;
    GtkWidget *hbox3;
    GtkWidget *separator;
    GtkWidget *event_offset;
    GtkWidget *event_trt;
    GtkWidget *event_label;
    GtkWidget *frame_elapsed;
    GtkWidget *frame_trt;
    GtkWidget *menubar;
    GtkWidget *menubar_filemenu;
    GtkWidget *menubar_fileitem;
    GtkWidget *menubar_helpmenu;
    GtkWidget *menubar_helpitem;
    GtkWidget *menubar_editmenu;
    GtkWidget *menubar_edititem;
    playback_control_t  *playctrl1;
    mpgedit_pcmview_ctx *pcmctx;
    idle_ctx            idlectx;
    global_vars_ctx     gvctx;
    long                play_sec   = 0;
    long                play_usec  = 0;
    GtkRequisition      widget_size;
    ledtime_cbctx_t     ledtime_lenctx;
    ledtime_cbctx_t     ledtime_trtctx;
    ledtime_cbctx_t     ledtime_offsetctx;
    xmpgedit_cmd_args_t *cmdline = NULL;

    _SET_X11_DISPLAY_NAME();

    memset(&idlectx,  0, sizeof(idlectx));
    memset(&gvctx,    0, sizeof(gvctx));

    /*
     * Make gvctx and playback control contexts reference each other
     */
    idlectx.gvctx  = &gvctx;
    gvctx.idlectx  = &idlectx;

    /*
     * Set configurable parameters in global context
     */
    read_config_file(CONFIG_FILE_NAME, &gvctx);

    init_rc_file(GTK_RC_FILE);
    gtk_init(&argc, &argv);

    /*
     * Process xmpgedit command line arguments
     */
    cmdline = _xmpgedit_parse_cmdline(&argc, argv);
    
    gvctx.tooltips = gtk_tooltips_new();
    gvctx.tooltips_entry = gtk_tooltips_new();
    gtk_tooltips_set_delay(gvctx.tooltips, 2000);
    gtk_tooltips_set_delay(gvctx.tooltips_entry, 2000);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window),
                         XMPGEDIT_PRODUCT " " XMPGEDIT_VERSION);

    gvctx.ledtrt = mpgedit_ledtime_new();
    gvctx.ledlen = mpgedit_ledtime_new();
    gvctx.ledoffset = mpgedit_ledtime_new();

    ledtime_trtctx.gvctx  = &gvctx;
    ledtime_trtctx.widget = gvctx.ledtrt;
    ledtime_lenctx.gvctx  = &gvctx;
    ledtime_lenctx.widget = gvctx.ledlen;
    ledtime_offsetctx.gvctx  = &gvctx;
    ledtime_offsetctx.widget = gvctx.ledoffset;

    /* Initialize the index queue if it does not exist */
    if (!gvctx.index_queue) {
        gvctx.index_queue = (mpgedit_workqueue_t *)
            calloc(1, sizeof(mpgedit_workqueue_t));
    }

    /* Initialize the decode queue if it does not exist */
    if (!gvctx.decode_queue) {
        gvctx.decode_queue = (mpgedit_workqueue_t *)
            calloc(1, sizeof(mpgedit_workqueue_t));
    }

    gvctx.main_window = window;

    mainvbox = gtk_vbox_new(FALSE, 0);
    mainhbox = gtk_hbox_new(FALSE, 0);
    box      = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(mainvbox);
    gtk_widget_show(mainhbox);
    gtk_widget_show(box);

    leftboxspacer = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(leftboxspacer);

    rightboxspacer = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(rightboxspacer);

    bottomboxspacer = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(bottomboxspacer);

    gtk_container_add(GTK_CONTAINER(window), mainvbox);
    gtk_container_set_border_width(GTK_CONTAINER(box), 0);

    /*
     * Start packing main menu bar into top-level container.  The "file" menu
     * is populated below, when more required widgets have been constructed.
     */
    menubar = gtk_menu_bar_new();
    gtk_widget_show(menubar);
    gtk_box_pack_start(GTK_BOX(mainvbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(mainvbox), mainhbox, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(mainhbox), leftboxspacer, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(mainhbox), box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mainhbox), rightboxspacer, FALSE, FALSE, 5);

    gvctx.editctx.scrolled_window = 
        ___make_scrolled_window(&idlectx.gvctx->editctx.clist);

    gvctx.editctx.offset_time = &gvctx.offset_time;
    gvctx.editctx.last_time   = &gvctx.trt_time;

    /*
     * Setup callbacks to handle mouse button press events, one to popup
     * the edit menu for right mouse button presses on the clist, the
     * other to popup the main menu for right mouse presses over any other
     * area over the main window.
     */
    gtk_widget_set_events(window, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(gvctx.editctx.clist), "select-row",
                     G_CALLBACK(cb_row_clicked), &gvctx);

    g_signal_connect(G_OBJECT(window), "button-press-event",
                     G_CALLBACK(cb_window_button_press), &gvctx);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(cb_destroy), &gvctx);

    g_signal_connect(G_OBJECT(window), "delete-event",
                     G_CALLBACK(cb_delete), &idlectx);

    gtk_box_pack_start(GTK_BOX(box),
                       gvctx.editctx.scrolled_window, TRUE, TRUE, 1);

    control_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(control_hbox);

    control_vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(control_vbox);
    gtk_box_pack_start(GTK_BOX(control_hbox), control_vbox, TRUE, TRUE, 0);

    box_fopen =  __make_widget_file_open(&gvctx);
    gtk_box_pack_start(GTK_BOX(box), box_fopen, FALSE, FALSE, 0);

    separator = gtk_hseparator_new();
    gtk_widget_show(separator);
    gtk_box_pack_start(GTK_BOX(box), separator, FALSE, FALSE, 8);

    /*
     * Elapsed, start, and TRT labels
     */
    box_elapsed = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_elapsed);

    frame_elapsed = gtk_frame_new("Playback time");
    gtk_widget_show(frame_elapsed);

    label = gvctx.ledlen->minsectime_init(gvctx.ledlen, 0, 0);
    idlectx.label_elapsed = label;
    gtk_widget_show(label);

    box_offset = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_offset);

    /*
     * Frame is created, but not shown; visibility is menu configurable
     * by OPTIONMENU_ACTION_OFFSET action.
     */
    gvctx.frame_offset = gtk_frame_new("Offset time");

    label_offset = gvctx.ledoffset->minsectime_init(gvctx.ledoffset, 0, 0);
    idlectx.label_offset = label_offset;
    gtk_widget_show(label_offset);
    
    if (cmdline && cmdline->offset) {
        gvctx.ledoffset->settime(gvctx.ledoffset,
                                 cmdline->offset_sec,
                                 cmdline->offset_msec);
        mpeg_time_init(&gvctx.offset_time,
                       cmdline->offset_sec,
                       cmdline->offset_msec);
    }

    /*
     * TRT label
     */
    box_trt = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_trt);

    frame_trt = gtk_frame_new("Last time");
    gtk_widget_show(frame_trt);
    gtk_container_set_border_width(GTK_CONTAINER(frame_trt), 0);

    label_trt = gvctx.ledtrt->minsectime_init(gvctx.ledtrt, 0, 0);
    gtk_widget_show(label_trt);

    event_offset = gtk_event_box_new();
    gtk_widget_show(event_offset);
    gtk_widget_set_events(event_offset, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(event_offset), "button_press_event",
                     G_CALLBACK(cb_ledtime), &ledtime_offsetctx);

    event_trt = gtk_event_box_new();
    gtk_widget_show(event_trt);
    gtk_widget_set_events(event_trt, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(event_trt), "button_press_event",
                     G_CALLBACK(cb_ledtime), &ledtime_trtctx);


    event_label = gtk_event_box_new();
    gtk_widget_show(event_label);
    gtk_widget_set_events(event_label, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(event_label), "button_press_event",
                     G_CALLBACK(cb_ledtime), &ledtime_lenctx);

    gtk_container_add(GTK_CONTAINER(event_offset), label_offset);
    gtk_container_add(GTK_CONTAINER(event_label), label);
    gtk_container_add(GTK_CONTAINER(event_trt),   label_trt);
    gtk_box_pack_start(GTK_BOX(box_offset), event_offset, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_elapsed), event_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_trt),     event_trt,   FALSE, FALSE, 0);
    idlectx.label_trt = event_trt;

    /*
     * File name entry widgets
     */
    box_entry = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_entry);
    gtk_container_set_border_width(GTK_CONTAINER(box_entry), 0);

    gtk_container_add(GTK_CONTAINER(gvctx.frame_offset), box_offset);

    gtk_container_add(GTK_CONTAINER(frame_elapsed), box_elapsed);
    gtk_box_pack_start(GTK_BOX(box_entry), frame_elapsed, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(frame_trt), box_trt);
    gtk_box_pack_end(GTK_BOX(box_entry),   frame_trt, FALSE, FALSE, 0);

    gvctx.volumectx = ___make_volume_control();
    gtk_widget_show_all(gvctx.volumectx->box);
    gtk_widget_hide(gvctx.volumectx->box);

    gtk_widget_size_request(gvctx.volumectx->scale, &widget_size);
    gtk_widget_set_size_request(gvctx.volumectx->scale,
                                -1, widget_size.height * 2);
    gtk_widget_size_request(window, &widget_size);

    /* Make the PCM viewer control */
    pcmctx           = ___make_pcmview_control(&gvctx);
    gvctx.pcmviewctx = pcmctx;
    gtk_widget_set_size_request(GTK_WIDGET(pcmctx->drawing_area),
                                widget_size.width, 150);

    /* Pack the PCM viewer control into the main box */
    gtk_box_pack_start(GTK_BOX(box), pcmctx->pcmview, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(box), box_entry,       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), gvctx.frame_offset, FALSE, FALSE, 0);

    /*
     * Start playback controls population
     */
    vbox_playback = gtk_vbox_new(FALSE, 5);
    gtk_widget_show(vbox_playback);
    hbox_playback = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox_playback);

    frame_playback = gtk_frame_new(NULL);
    gtk_widget_show(frame_playback);
    gtk_container_add(GTK_CONTAINER(frame_playback), control_hbox);
    gtk_box_pack_start(GTK_BOX(box), frame_playback, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(control_vbox),  hbox_playback, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_playback), vbox_playback, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(control_hbox), gvctx.volumectx->box, 
                       FALSE, FALSE, 5);
                      
    playctrl1 = __construct_playback_control(NULL,
                                             play_sec, play_usec);
    gtk_widget_show(playctrl1->playctrl);
    gtk_box_pack_start(GTK_BOX(vbox_playback),
                       playctrl1->playctrl, TRUE, TRUE, 0);

    playctrl1->sbctx.hadj_scale = g_signal_connect(
        G_OBJECT(playctrl1->adj_scale), "value_changed",
        G_CALLBACK(cb_scrollbar_adj), &idlectx);
    g_signal_connect(G_OBJECT(playctrl1->button_playto), "clicked",
                     G_CALLBACK(cb_playto_button), &idlectx);
    g_signal_connect(G_OBJECT(playctrl1->button_pause), "clicked", 
                     G_CALLBACK(cb_pause_button), &idlectx);
    g_signal_connect(G_OBJECT(playctrl1->button_stop), "clicked",
                     G_CALLBACK(cb_stop_button), &idlectx);
    g_signal_connect(G_OBJECT(playctrl1->button_play), "clicked", 
                     G_CALLBACK(cb_play_button), &idlectx);
    recbtn_ctx = calloc(1, sizeof(record_button_ctx_t));
    if (recbtn_ctx) {
        recbtn_ctx->start = TRUE;
        recbtn_ctx->gvctx = &gvctx;
    }
    g_signal_connect(G_OBJECT(playctrl1->button_recstart), "clicked", 
                     G_CALLBACK(cb_record_time), recbtn_ctx);

    recbtn_ctx = calloc(1, sizeof(record_button_ctx_t));
    if (recbtn_ctx) {
        recbtn_ctx->start = FALSE;
        recbtn_ctx->gvctx = &gvctx;
    }
    g_signal_connect(G_OBJECT(playctrl1->button_recstop), "clicked", 
                     G_CALLBACK(cb_record_time), recbtn_ctx);

    gtk_tooltips_set_tip(gvctx.tooltips, GTK_WIDGET(playctrl1->button_playto),
                         TIP_PLAYTO, NULL);
    gtk_tooltips_set_tip(gvctx.tooltips, GTK_WIDGET(playctrl1->button_pause),
                         TIP_PAUSE, NULL);
    gtk_tooltips_set_tip(gvctx.tooltips, GTK_WIDGET(playctrl1->button_stop),
                         TIP_STOP, NULL);
    gtk_tooltips_set_tip(gvctx.tooltips, GTK_WIDGET(playctrl1->button_play),
                         TIP_PLAY, NULL);
    gtk_tooltips_set_tip(gvctx.tooltips, GTK_WIDGET(playctrl1->button_recstart),
                         TIP_RECORD_START, NULL);
    gtk_tooltips_set_tip(gvctx.tooltips, GTK_WIDGET(playctrl1->button_recstop),
                         TIP_RECORD_STOP, NULL);

    idlectx.player = playctrl1;

    /*
     * End of playback controls population
     */

    hbox3 = gtk_hbox_new(TRUE, 4);
    gtk_widget_show(hbox3);

    gtk_box_pack_start(GTK_BOX(box), hbox3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), bottomboxspacer, FALSE, FALSE, 5);

    /*
     * Populate the edit window with files named from the command line,
     * and create pop-up menu structures.
     */
    gvctx.main_menu = ___mainmenu_make(&gvctx);
    gvctx.edit_menu = ___editmenu_make(&gvctx);

    ___set_editor_defaults(&gvctx);
    ___load_editor_blank_first_line(&gvctx);
    if (cmdline && cmdline->editspec) {
        ___load_editor_by_editspec(
            &gvctx, cmdline->editspec, 
            mpgedit_editspec_get_length(cmdline->editspec));
    }
    else if (argc > 1) {
        ___load_editor_by_files(&gvctx, &argv[1], argc-1);
        gtk_clist_select_row(GTK_CLIST(gvctx.editctx.clist), argc-1, 0);
    }
    /*
     * file menu creation. adam: This is the same menu as
     * used for the popup.  I did not think this was allowed,
     * but it seems to work fine.
     */
    menubar_filemenu = gvctx.main_menu;
    menubar_fileitem = gtk_menu_item_new_with_label("File");
    gtk_widget_show(menubar_fileitem);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar_fileitem),
                              menubar_filemenu);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), menubar_fileitem);

    /*
     * Edit menu creation. adam: This is the same menu as
     * used for the popup.  I did not think this was allowed,
     * but it seems to work fine.
     */
    menubar_editmenu = gvctx.edit_menu;
    menubar_edititem = gtk_menu_item_new_with_label("Edit");
    gtk_widget_show(menubar_edititem);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar_edititem),
                              menubar_editmenu);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), menubar_edititem);

    /* Help menu creation */
    menubar_helpmenu = ___helpmenu_make(&gvctx);
    menubar_helpitem = gtk_menu_item_new_with_label("Help");
    gtk_widget_show(menubar_helpitem);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menubar_helpitem),
                              menubar_helpmenu);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), menubar_helpitem);
    gtk_menu_item_right_justify(GTK_MENU_ITEM(menubar_helpitem));

    /* About dialog creation */
    gvctx.about_dialog = ___aboutdialog_make(&gvctx);
    gtk_widget_set_size_request(GTK_WIDGET(window), 500, -1);

    gtk_widget_show(window);
    gtk_main();
    return 0;
}

_MAIN(main)
