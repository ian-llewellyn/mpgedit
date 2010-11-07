#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "playctrl.h"

#include "editor.h"

#define _FUZZYCMP(a,b,c) \
    ((a)==(b) || ((a)>(b) && ((a)-(b))<(c)) || ((b)>(a) && ((b)-(a))<(c)))

int  ___row_init(edit_row_ctx *er_data, 
                 char *file_name,
                 int s_min, int s_sec, int s_msec,
                 int e_min, int e_sec, int e_msec)
{
    char buf[128];
    int  changed = 0;
    int  slabel   = 0;
    int  elabel   = 0;

    if (!er_data) {
        return changed; 
    }

    if (file_name) {
        if (er_data->file_name) {
            free(er_data->file_name);
        }
        er_data->file_name = strdup(file_name);
    }

    if (s_min != -1 && s_sec != -1 && s_msec != -1 &&
        !(s_min == er_data->s_min &&
          s_sec == er_data->s_sec &&
          _FUZZYCMP(s_msec,  er_data->s_msec, 3)))
    {
        changed              = 1;
        slabel               = 1;
        er_data->s_min       = s_min;
        er_data->s_sec       = s_sec;
        er_data->s_msec      = s_msec;
    }
    else if (s_min == 0 && s_sec == 0 && s_msec == 0) {
        slabel = 1;
    }

    if (e_min != -1 && e_sec != -1 && e_msec != -1 &&
        !(e_min == er_data->e_min &&
          e_sec == er_data->e_sec &&
          _FUZZYCMP(e_msec,  er_data->e_msec, 3)))
    {
        changed              = 1;
        elabel               = 1;
        er_data->e_min       = e_min;
        er_data->e_sec       = e_sec;
        er_data->e_msec      = e_msec;
    }
    else if (e_min == 0 && e_sec == 0 && e_msec == 0) {
        elabel = 1;
    }

    if (slabel) {
        sprintf(buf, "%4d:%02d:%03d", s_min, s_sec, s_msec);
        if (er_data->stime_str) {
            free(er_data->stime_str);
        }
        er_data->stime_str = strdup(buf);
    }
    if (elabel) {
        sprintf(buf, "%4d:%02d:%03d", e_min, e_sec, e_msec);
        if (er_data->etime_str) {
            free(er_data->etime_str);
        }
        er_data->etime_str = strdup(buf);
    }
    return changed;
}


edit_row_ctx *___row_make(char *file, int indx, 
                       int s_min, int s_sec, int s_msec,
                       int e_min, int e_sec, int e_msec, gpointer ctx)
{
    edit_row_ctx *er_data;

    er_data              = calloc(1, sizeof(edit_row_ctx));
    er_data->row_num     = indx;
    er_data->ctx         = ctx;

    ___row_init(er_data, file,
                s_min, s_sec, s_msec,
                e_min, e_sec, e_msec);
    return er_data;
}


void ___row_update(edit_ctx *edctx,
                   GtkWidget *clist)
{
    edit_row_ctx *er_data;
    int          row;
    mpeg_time    stime;
    mpeg_time    etime;
    mpeg_time    dtime;
    char         duration[128];

    if (!edctx || !edctx->cursor) {
        return;
    }
    er_data = edctx->cursor;
    row     = er_data->row_num;

    mpeg_time_string2time(er_data->stime_str, &stime);
    if (edctx->cursor == edctx->tail &&
        edctx->cursor->e_sec == 0 &&
        edctx->cursor->e_msec == 0)
    {
        sprintf(er_data->etime_str, "%ld:%02ld:%03ld", 
                mpeg_time_getsec(edctx->last_time)/60, 
                mpeg_time_getsec(edctx->last_time)%60, 
                mpeg_time_getusec(edctx->last_time));
        dtime = mpeg_time_compute_delta(stime.units, stime.usec, 
                                        mpeg_time_getsec(edctx->last_time),
                                        mpeg_time_getusec(edctx->last_time));
    }
    else {
        mpeg_time_string2time(er_data->etime_str, &etime);
        dtime = mpeg_time_compute_delta(stime.units, stime.usec,
                                        etime.units, etime.usec);
    }

    duration[0] = '\0';
    sprintf(duration, "%ld:%02ld:%03ld", dtime.units/60,
            dtime.units%60, dtime.usec/1000);

    gtk_clist_set_text(GTK_CLIST(clist), row, 0, er_data->file_name);
    gtk_clist_set_text(GTK_CLIST(clist), row, 1, er_data->stime_str);
    gtk_clist_set_text(GTK_CLIST(clist), row, 2, er_data->etime_str);
    gtk_clist_set_text(GTK_CLIST(clist), row, 3, duration);
    edctx->dirty = 1;
}


void ___row_free(edit_row_ctx *er)
{
    if (!er) {
        return;
    }
    if (er->file_name) {
        free(er->file_name);
    }

    if (er->stime_str) {
        free(er->stime_str);
    }

    if (er->etime_str) {
        free(er->etime_str);
    }
    free(er);
}


GtkWidget *___make_scrolled_window(GtkWidget **rlist)
{
    GtkWidget *scrolled_window;
    GtkWidget *list;
    char *list_titles[4];
    GtkWidget *hbox;
    GtkWidget *image;
    GtkWidget *label;

    /*
     * create a new scrolled window.
     */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrolled_window);
    
    list_titles[0] = "File name";
    list_titles[1] = "Start Time";
    list_titles[2] = "End Time";
    list_titles[3] = "Duration";
    list = gtk_clist_new_with_titles(4, list_titles);
    gtk_widget_show(list);

    gtk_container_add(GTK_CONTAINER(scrolled_window), list);

    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 1, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 2, TRUE);
    gtk_clist_column_titles_passive(GTK_CLIST(list));

    gtk_clist_set_button_actions(GTK_CLIST(list), 0, GTK_BUTTON_SELECTS);
    gtk_clist_set_button_actions(GTK_CLIST(list), 1, GTK_BUTTON_SELECTS);
    gtk_clist_set_button_actions(GTK_CLIST(list), 2, GTK_BUTTON_SELECTS);

    /* Set CList titles with new labels that contain the current text,
     * and a pixmap of the start/stop buttons.
     */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);

    image = xpm_pixmap_box(gtk_settings_get_default(), IMAGE_RECORD_START);
    gtk_widget_show(image);

    label = gtk_label_new("Start Time");
    gtk_widget_show(label);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, 0, 5);
    gtk_clist_set_column_widget(GTK_CLIST(list), 1, hbox);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);

    image = xpm_pixmap_box(gtk_settings_get_default(), IMAGE_RECORD_STOP);
    gtk_widget_show(image);

    label = gtk_label_new("End Time");
    gtk_widget_show(label);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE,  0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE,  0, 5);
    gtk_clist_set_column_widget(GTK_CLIST(list), 2, hbox);

    gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_BROWSE);
    
    *rlist = list;
    return scrolled_window;
}


char *___get_default_abandoned_filename(char *name)
{
    FILE  *fp;
    char *rname;
    char *cp;

    if (name) {
        rname = name;
    }
    else {
        rname = (char *) malloc(MAXPATHLEN);
        if (!rname) {
            return NULL;
        }
    }

    fp = fopen(ABANDONED_FILENAME_FILE, "rb");
    if (!fp) {
        strcpy(rname, DEFAULT_ABANDONED_FILENAME);
    }
    else {
        cp = fgets(rname, MAXPATHLEN, fp);
        fclose(fp);
        if (!cp) {
            return NULL;
        }
    }
    return rname;
}


int ___put_default_abandoned_filename(char *name)
{
    FILE *fp;
    int sts;

    if (!name) {
        return EOF;
    }

    fp = fopen(ABANDONED_FILENAME_FILE, "wb");
    if (!fp) {
        return EOF;
    }
    sts = fputs(name, fp);
    fclose(fp);
    return sts;
}


/* 
 * Sanity check file data.  Read first line, and look for
 * proper format; %%data%%.  This is not foolproof, but it is
 * better than nothing.
 */
static int ___verify_abandoned_edits_line(char *str)
{
    char *cp;

    if (!str) {
        return 0;
    }
    cp = strstr(str, "%%");
    if (!cp) {
        return 0;
    }
    cp += 2;
    cp = strstr(cp, "%%");
    if (!cp) {
        return 0;
    }
    return 1;
}


int ___read_abandoned_edits(char *file_name, editspec_t **redarray)
{
    editspec_t *edarray = NULL;
    int edlen;
    int sts = 0;
    FILE *fp = NULL;
    char *str = NULL;
    int str_len = MAXPATHLEN + 64;
    char *cp;
    char *end;
    char *ep;
    int tval;
    char *dir = NULL;
    char *filename;
    char timespec[128];

    fp = fopen(file_name, "r");
    if (!fp) {
        goto clean_exit;
    }

    /*
     * Save directory path prefix to saved file.  This will be used as
     * path prefix to edits for files not named with a path.
     */
    cp = strrchr(file_name, '/');
    if (cp) {
        dir = (char *) calloc(1, cp - file_name + 2);
        if (dir) {
            strncat(dir, file_name, cp - file_name + 1);
        }
    }

    str = (char *) malloc(str_len);
    if (!str) {
        goto clean_exit;
    }

    /* Count number of lines in input file */
    edlen = 0;
    while (!feof(fp)) {
        cp = fgets(str, str_len-1, fp);
        if (cp) {
            if (!___verify_abandoned_edits_line(cp)) {
                goto clean_exit;
            }
            edlen++;
        }
    }
    rewind(fp);
    edarray = mpgedit_editspec_init();
    if (!edarray) {
        goto clean_exit;
    }

    edlen = 0;
    while (!feof(fp)) {
        filename = NULL;
        cp = fgets(str, str_len-1, fp);
        if (cp) {
            cp[strlen(cp)-1] = '\0';

            /* Extract file name */
            end = strstr(cp, "%%");
            if (end) {
                *end = '\0';

                /*
                 * Check for path to filename.  If not present, prefix with
                 * any path to abandoned file name.  Assumption here is the
                 * files being edited reside in the same location as the
                 * abandoned edit file, when no path prefix is present.
                 */
                if (!strchr(cp, '/') && dir) {
                    filename = (char *) malloc(strlen(cp) + strlen(dir) + 1);
                    strcpy(filename, dir);
                    strcat(filename, cp);
                }
                else {
                    filename = strdup(cp);
                }
            }
            cp = end+2;
            while (*cp && isspace(*cp)) {
                cp++;
            }

            /* Extract stime */
            end = strstr(cp, "%%");
            if (end) {
                *end = '\0';
            }
         
            tval = 0;
            ep = strchr(cp, ':');
            if (ep) {
                *ep = '\0';
                tval = atoi(cp) * 60;
                cp = ep + 1;
            }
            ep = strchr(cp, '.');
            if (ep) {
                *ep = '\0';
                tval += atoi(cp);
                cp = ep + 1;
            }
            sprintf(timespec, "%d.%d-", tval, atoi(cp));
            cp = end+2;
            while (*cp && isspace(*cp)) {
                cp++;
            }

            /* Extract etime */
            end = strstr(cp, "%%");
            if (end) {
                *end = '\0';
            }
         
            tval = 0;
            ep = strchr(cp, ':');
            if (ep) {
                *ep = '\0';
                tval = atoi(cp) * 60;
                cp = ep + 1;
            }
            ep = strchr(cp, '.');
            if (ep) {
                *ep = '\0';
                tval += atoi(cp);
                cp = ep + 1;
            }
            sprintf(&timespec[strlen(timespec)], "%d.%d", tval, atoi(cp));
            mpgedit_editspec_append(edarray, filename, timespec);
            edlen++;
        }
    }


    sts = edlen;
clean_exit:
    if (fp) {
        fclose(fp);
    }

    if (str) {
        free(str);
    }
    *redarray = edarray;
    return sts;
}


int ___write_abandoned_edits(char *file_name,
                             int force,
                             editspec_t *edarray, int edlen)
{
    FILE *fp = NULL;
    int i;
    int sts = 0;
    char      *filename;
    long      ssec;
    long      susec;
    long      esec;
    long      eusec;

    /*
     * Test for file existence.  Is an error when force is not in effect.
     */
    if (!force) {
        fp = fopen(file_name, "r");
        if (fp) {
            goto clean_exit;
        }
    }

    fp = fopen(file_name, "w");
    if (!fp) {
        goto clean_exit;
    }

    for (i=0; i<edlen; i++) {
        filename = mpgedit_editspec_get_file(edarray, i);
        mpeg_time_gettime(mpgedit_editspec_get_stime(edarray, i),
                          &ssec, &susec);
        mpeg_time_gettime(mpgedit_editspec_get_etime(edarray, i),
                          &esec, &eusec);
        fprintf(fp, "%s%%%%%ld:%ld.%ld%%%%%ld:%ld.%ld\n", 
                filename,
                ssec / 60,
                ssec % 60,
                susec  /1000,
                esec / 60,
                esec % 60,
                eusec  / 1000);
    }
    sts = i;

clean_exit:
    if (fp) {
        fclose(fp);
    }

    return sts;
}


void ___edit_row_list_renumber(edit_row_ctx *row, int row_num)
{
    while ((row = row->next)) {
        row_num++;
        row->row_num = row_num;
    }
}


/*
 * Add an edit row to the current cursor position.  Adds above the cursor
 * position when "above" is true, below otherwise.
 */
void ___edit_row_list_insert_cursor(edit_row_ctx *row,
                                    edit_ctx *edctx,
                                    int above)
{ 
    if (!edctx->head) {
        ___edit_row_list_append(row, edctx);
    }
    else if (above) {
        if (edctx->cursor == edctx->head) {
            /*
             * Row is now the new head of the list
             */
            row->next           = edctx->cursor;
            row->prev           = NULL;
            edctx->head         = row;
            row->row_num        = 0;
            edctx->cursor->prev = row;
            edctx->cursor       = row;
            ___edit_row_list_renumber(row, row->row_num);
        }
        else {
            row->row_num              = edctx->cursor->row_num;
            row->next                 = edctx->cursor;
            row->prev                 = edctx->cursor->prev;
            edctx->cursor->prev->next = row;
            edctx->cursor->prev       = row;
            edctx->cursor             = row;
            ___edit_row_list_renumber(row, row->row_num);
        }
    }
    else {
        /*
         * Add row below cursor position
         */
        if (edctx->cursor == edctx->tail) {
            row->next           = edctx->cursor->next;
            row->prev           = edctx->cursor;
            edctx->cursor->next = row;
            edctx->tail         = row;
            edctx->cursor       = row;
            if (row->prev) {
                row->row_num = row->prev->row_num + 1;
            }
        }
        else {
            row->row_num              = edctx->cursor->row_num + 1;
            row->next                 = edctx->cursor->next;
            row->prev                 = edctx->cursor;
            edctx->cursor->next->prev = row;
            edctx->cursor->next       = row;
            edctx->cursor             = row;
            ___edit_row_list_renumber(row, row->row_num);
        }
    }
}


/*
 * Add an edit row to the end of the edit list
 */
void ___edit_row_list_append(edit_row_ctx *row, edit_ctx *edctx)
{
    if (!edctx->head) {
        edctx->head = row;
        edctx->tail = row;
        row->next   = NULL;
        row->prev   = NULL;
    }
    else {
        row->next         = edctx->tail->next;
        row->prev         = edctx->tail;
        edctx->tail->next = row;
        edctx->tail       = row;
        row->row_num      = edctx->tail->prev->row_num+1;
    }
    edctx->cursor = row;
}


/* 
 * Remove the current edit row at the cursor position
 */
edit_row_ctx *___edit_row_list_delete(edit_ctx *edctx)
{
    edit_row_ctx *deleted;

    if (!edctx->cursor) {
        return NULL;
    }
    deleted = edctx->cursor;
    if (edctx->cursor == edctx->head) {
        if (edctx->head->next) {
            edctx->head->next->prev = edctx->head->prev;
        }
        else {
            /* Last node in list was deleted */
            edctx->tail = NULL;
        }
        edctx->head             = edctx->head->next;
        edctx->cursor           = edctx->head;
        if (edctx->cursor) {
            edctx->cursor->row_num--;
            ___edit_row_list_renumber(edctx->cursor, edctx->cursor->row_num);
        }
        
    }
    else if (edctx->cursor == edctx->tail) {
        edctx->tail->prev->next = edctx->tail->next;
        edctx->tail             = edctx->tail->prev;
        edctx->cursor           = edctx->tail;
    }
    else {
        edctx->cursor->next->prev = edctx->cursor->prev;
        edctx->cursor->prev->next = edctx->cursor->next;
        edctx->cursor             = edctx->cursor->next;
        edctx->cursor->row_num--;
        ___edit_row_list_renumber(edctx->cursor, edctx->cursor->row_num);
    }
    return deleted;
}


void ___edit_row_list_iterate(
    edit_ctx *edctx,
    void (*func)(void *ctx, edit_row_ctx *row), void *ctx)
{
    edit_row_ctx *row;

    row = edctx->head;
    while (row) {
        func(ctx, row);
        row = row->next;
    }
}


/*
 * This can be painfully slow when many edits are present in the editor.
 * Maybe have an array of pointers to edit_row_ctx that is indexed by
 * the desired "index" value, to replace the list search will be a good idea.
 */
edit_row_ctx *___edit_row_find_index(edit_ctx *edctx, int index)
{
    edit_row_ctx *row = NULL;

    row = edctx->head;
    while (row) {
        if (row->row_num == index) {
            break;
        }
        row = row->next;
    }
    return row;
}


#ifdef _UNIT_TEST

void destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}



void cb_row_clicked(GtkWidget *widget, int row, int column,
                    GdkEventButton *event, gpointer data)
{
    printf("cb_row_clicked: called %d %d\n", row, column);
}



int main(int argc, char *argv[])
{
    GtkWidget    *dialog;
    GtkWidget    *scrolled_window;
    GtkWidget    *button;
    edit_row_ctx *row;
    GtkWidget    *clist;
    GtkWidget    *event_box;
    char         *item[3];

    int i;
    
    gtk_init(&argc, &argv);
    
    /*
     * Create a new dialog window for the scrolled window to be packed into.
     *
     */
    dialog = gtk_dialog_new();
    g_signal_connect(G_OBJECT(dialog), "destroy",
                      G_CALLBACK(destroy), NULL);
    gtk_window_set_title(GTK_WINDOW(dialog), "GtkScrolledWindow example");
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 0);
    gtk_widget_set_size_request(dialog, 300, 300);

    scrolled_window = ___make_scrolled_window(&clist, &event_box);
    g_signal_connect(GTK_CLIST(clist), "select_row",
                     G_CALLBACK(cb_row_clicked), NULL);

        
    /* The dialog window is created with a vbox packed into it. */                                                              
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, 
                        TRUE, TRUE, 0);

    /* Add a "close" button to the bottom of the dialog */
    button = gtk_button_new_with_label("close");
    g_signal_connect_swapped(G_OBJECT(button), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             dialog);
    
    /* this makes it so the button is the default. */
    
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
                        button, TRUE, TRUE, 0);
    
    /* This grabs this button to be the default button. Simply hitting
     * the "Enter" key will cause this button to activate. */
    gtk_widget_grab_default(button);
    gtk_widget_show(button);

    /*
     * this simply creates a grid of toggle buttons on the table to
     * demonstrate the scrolled window.
     */
    for(i = 0; i < 20; i++) {
        row = ___row_make("file name", i, 
                          i, i*2, i*3-5, 
                          i, i*2, i*3-5, NULL);
        item[0] = row->file_name;
        item[1] = row->stime_str;
        item[2] = row->etime_str;
        gtk_clist_append(GTK_CLIST(clist), item);

    }
    
    gtk_widget_show(dialog);
    
    gtk_main();
    
    return 0;
}
#endif
