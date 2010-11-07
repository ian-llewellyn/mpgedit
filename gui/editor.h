#ifndef _EDITOR_H_
#define _EDITOR_H_

#include <editif.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#define ABANDONED_FILENAME_FILE    ".mp3edit_abandonded_name"
#define DEFAULT_ABANDONED_FILENAME ".mp3edit_abandonded"

typedef struct _edit_row_ctx {
    gpointer  ctx;
    int       row_num;
    int       col_num;
    int       indexing;
    char      *file_name;
    char      *stime_str;
    char      *etime_str;
    int       s_min;
    int       s_sec;
    int       s_msec;
    int       e_min;
    int       e_sec;
    int       e_msec;
    struct _edit_row_ctx *next;
    struct _edit_row_ctx *prev;
} edit_row_ctx;


typedef struct _edit_ctx {
    GtkWidget    *clist;           /* List of edits */
    edit_row_ctx *head;            /* Head of edit_row list */
    edit_row_ctx *cursor;          /* Currently selected edit row in list */
    edit_row_ctx *tail;            /* Tail of edit_row list */
    edit_row_ctx *deleted;         /* Item deleted or copied */
    GtkWidget    *scrolled_window; /* Scrolled window containing edits vbox */
    mpeg_time    *offset_time;     /* reference to gvctx offset_time */
    mpeg_time    *last_time;       /* reference to gvctx offset_time */
    int          dirty;            /* Set when editor has been modified */
} edit_ctx;

edit_row_ctx *___row_make(char *file, int indx,
                           int s_min, int s_sec, int s_msec,
                           int e_min, int e_sec, int e_msec, gpointer ctx);


GtkWidget *___make_scrolled_window(GtkWidget **rlist);


void ___row_update(edit_ctx *edctx,
                   GtkWidget *clist);

int ___row_init(edit_row_ctx *er_data, 
                 char *file,
                 int s_min, int s_sec, int s_msec,
                 int e_min, int e_sec, int e_msec);

void ___row_free(edit_row_ctx *er);

char *___get_default_abandoned_filename(char *name);

int ___put_default_abandoned_filename(char *name);

int ___read_abandoned_edits(char *file_name, editspec_t **redarray); 

int ___write_abandoned_edits(char *file_name,
                             int force,
                             editspec_t *edarray, int edlen);


void ___edit_row_list_renumber(edit_row_ctx *row, int row_num);

void ___edit_row_list_insert_cursor(edit_row_ctx *row,
                                    edit_ctx *edctx,
                                    int above);
void ___edit_row_list_append(edit_row_ctx *row, edit_ctx *edctx);

edit_row_ctx *___edit_row_list_delete(edit_ctx *edctx);

void ___edit_row_list_iterate(
    edit_ctx *edctx,
    void (*func)(void *ctx, edit_row_ctx *row), void *ctx);

edit_row_ctx *___edit_row_find_index(edit_ctx *edctx, int index);

#endif
