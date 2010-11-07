#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <gtk/gtk.h>
#include "editor.h"
#include "volume.h"
#include "pcmview.h"
#include "playctrl.h"
#include "ledtime.h"


typedef struct _outfile_dialog_ctx {
    struct _global_vars_ctx *gvctx;
    GtkWidget       *entry;
    GtkWidget       *dialog;
#if 1 /* ?? */
    GtkWidget       *radio_split;
#endif
    int             radio_selection;
    int             action;
    char            **file;
    int             *radio_action;
} outfile_dialog_ctx;


/*
 * Function to call after ___row_modified_idleproc() has completed.
 */
typedef struct _row_modified_dialog_ctx {
    void      (*proc)(void *data);
    GtkWidget *widget;
    GdkEvent  *event;
    void      *data;
} row_modified_dialog_ctx;


typedef struct _global_vars_ctx {
    GtkWidget *main_window;
    GtkWidget *edit_menu;
    GtkWidget *main_menu;
    GtkWidget *about_dialog;
    GtkWidget *entry;
    GtkWidget  *open_file_toggle;

    /*
     * Widgets accessed after creation
     */
    GtkWidget *menu_edit_pbelow;
    GtkWidget *menu_save_session;
    GtkWidget *menu_edit_cut;
    GtkWidget *menu_edit_copy;
    GtkWidget *menu_edit_chain;
    GtkWidget *menu_edit_duplicate;
    GtkWidget *menu_edit_stime;
    GtkWidget *menu_edit_time;
    GtkWidget *menu_edit;

    GtkTooltips *tooltips;
    GtkTooltips *tooltips_entry;
    edit_ctx  editctx;
    volume_control_ctx *volumectx;
    mpgedit_pcmview_ctx *pcmviewctx;
    outfile_dialog_ctx *ofdialogctx;
    outfile_dialog_ctx *abandon_file_ctx;
    outfile_dialog_ctx *query_abandon_ctx;

    /* LED contexts */
    mpgedit_ledtime_t *ledtrt;
    mpgedit_ledtime_t *ledlen;
    mpgedit_ledtime_t *ledoffset;
    GtkWidget         *frame_offset; /* Container for ledoffset */
    GtkWidget         *menu_offset;  /* "Set Offset" menu item  */
    GtkWidget         *menu_offset_sep;  /* menu_offset spearator */

    mpeg_time offset_time;
    mpeg_time trt_time;

    /* File selection */
    GtkWidget *fs;

    /* PCM Pick level */
    mpgedit_ledtime_t *ledpcm;

    /* Toolkit properties, needed for pixmap lookup */
    GtkSettings *settings;

    row_modified_dialog_ctx row_modified_ctx;

    /* 
     * Back reference to both playback controls.
     */
    struct _idle_ctx  *idlectx;

    /*
     * File indexing parameters
     */
    int                     idle_indexfile_count;
    int                     idle_decodefile_count;

    /* Put files onto this queue that need indexing */
    struct _mpgedit_workqueue_t *index_queue;

    /* Put files onto this queue that need decoding into PCM levels */
    struct _mpgedit_workqueue_t *decode_queue;

    int edit_action_abandon_save;
    int ignore_scrollbar_set;

    /*
     * File selection default directory path.  Remember last path picked.
     */
    char *filesel_path;
    char *filesel_pattern;

    /*
     * Configurable values
     */
    int pause_visable;
    int stop_visable;
    int volume_visable;
    int offset_visable;
    char *output_file;
    int  output_action;
    char *abandon_file;
    char *stats_str;
} global_vars_ctx;


/* 
 * This entire structure probably should be part of the playback_control_t
 * Will refactor that later...
 */
typedef struct _idle_ctx {
    GtkWidget           *label_elapsed;
    GtkWidget           *label_offset;
    GtkWidget           *label_trt;

    struct _playback_control_t  *player;

    int                 adj_elapsed_label_noset;
    int                 block_pcmview_seek;
    gint                idle_tag;
    void                *playctx;
    char                *play_file;
    
    int                 stime_sec;
    int                 stime_usec;

    int                 save_stime_sec;
    int                 save_stime_usec;

    int                 elapsed_sec;
    int                 elapsed_usec;

    int                 pause_sec;
    int                 pause_usec;

    int                 saved_sec;
    int                 saved_usec;

    int                 playto_sec;
    int                 playto_msec;
    int                 dirty;
    int                 dirty_saved;
    int                 dirty_sec;
    int                 dirty_usec;
    edit_row_ctx        *dirty_row;
    global_vars_ctx     *gvctx;
} idle_ctx;


#endif
