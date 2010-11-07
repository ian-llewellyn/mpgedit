#include "playctrl.h"
#include "player.h"


#define _MAX_PIXMAP_PATH 128

static char *default_pixmap_paths[] = {
 ".",
 "/usr/share/pixmaps/xmpgedit",
 "/usr/local/share/xmpgedit",
 "/opt/local/share/xmpgedit",               /* For Mac OSX install       */
 "/sw/share/xmpgedit",                      /* For Mac OSX Fink install  */
 "/opt/local/xmpgedit/pixmaps",             /* Not used yet by installer */
 "/opt/local/mpgedit.org/pixmaps",          /* Not used yet by installer */
 "c:/program files/mpgedit.org/xmpgedit",
 NULL
};


static char *valid_pixmap_path(char *inpath, char *file)
{
    char path[_MAX_PIXMAP_PATH];

    strcpy(path, inpath);
    strcat(path, "/");
    strcat(path, file);
    if (access(path, F_OK) == 0) {
        return strdup(path);
    }
    else {
        return NULL;
    }
}


static char *_get_image_file_path(char *file, char *paths[])
{
    char *env;
    char *retpath = NULL;
    int  i;

    env = getenv("_GTK_PIXMAP_PATH");
    if (env && (retpath = valid_pixmap_path(env, file))) {
        return retpath;
    }

    for (i=0; paths[i]; i++) {
        if ((retpath = valid_pixmap_path(paths[i], file))) {
            return retpath;
        }
    }
    return NULL;
}


GtkWidget *xpm_pixmap(GtkSettings *settings, gchar *xpm_filename)
{
     GtkWidget *image;
     char      *xpm_file = NULL;

     if (!settings) {
         settings = gtk_settings_get_default();
     }
     xpm_file = _get_image_file_path(xpm_filename, default_pixmap_paths);
     if (!xpm_file) {
         xpm_file = gtk_rc_find_pixmap_in_path(settings,
                                               NULL, xpm_filename);
         if (!xpm_file) {
             xpm_file = strdup(xpm_filename);
         }
     }

     /* Now on to the image stuff */
#ifdef _DEBUG
         printf("xpm_pixmap: %s=<%s>\n", xpm_filename, xpm_file);
#endif
     image = gtk_image_new_from_file(xpm_file);
#ifndef WIN32
     /*
      * adam/TBD: Hmm, the absence of g_free on Win32 makes me wonder.
      * Calling free() crashes on Win32.  Maybe the memory being allocated in
      * one DLL and being freed by another is the problem.  Don't know.  This
      * will just have to be a memory leak for now.
      */
     g_free(xpm_file);
#endif
     gtk_widget_show(image);
     return image;
}


GtkWidget *xpm_pixmap_box(GtkSettings *settings, gchar *xpm_filename)
{
     GtkWidget *box;
     GtkWidget *image;

     /* Create box for image and label */
     box = gtk_hbox_new(FALSE, 0);
     gtk_container_set_border_width(GTK_CONTAINER(box), 0);

     image = xpm_pixmap(settings, xpm_filename);
     gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
     gtk_widget_show(image);

     return box;
}


static void ___set_spin_buttons_value(spin_button_ctx_t *sbctx, gdouble value)
{
    int min_value;
    int sec_value;
    int msec_value;

    /* Column values: 100min:60s:1000ms */
    min_value  = ((int) (value / (60.0*1000.0)));
    sec_value  = ((int) (value - min_value*60.0*1000.0))/1000;
    msec_value = ((int) (value - min_value*60.0*1000.0 - sec_value*1000.0));

    /*
     * Must set spinner adjustments in this order, otherwise next higher
     * button jumps by 2 counts.  Not sure why this is.
     */
    gtk_adjustment_set_value(sbctx->adj_msec, (gdouble) msec_value);
    gtk_adjustment_set_value(sbctx->adj_sec,  (gdouble) sec_value);
    gtk_adjustment_set_value(sbctx->adj_min,  (gdouble) min_value);
}


void ___spin_button_set_value(spin_button_ctx_t *sbctx, int insec, int inmsec)
{
    ___set_spin_buttons_value(sbctx, 
                             (gdouble) insec * 1000.0 + (gdouble) inmsec);
}


gdouble ___scale_set_value(spin_button_ctx_t *sbctx, long sec, long msec)
{
    gdouble sum = 0.0;

    sum += sec * 1000.0;
    sum += msec;
    gtk_adjustment_set_value(sbctx->adj_scale, sum);
    return sum;
}


void ___set_scale_spin_buttons_range_from_time(playback_control_t *player,
                                               long              sec,
                                               long              msec)
{
    gdouble       upper_min;
    gdouble       upper_sec;
    gdouble       upper_msec;

    GTK_ADJUSTMENT(player->playtime_adj)->upper = 
        (gdouble) sec*1000.0 + (gdouble) msec + 1.0;

    upper_min  = (gdouble) (sec/60);
    upper_sec  = (gdouble) (upper_min==0) ? (sec%60) : 60;
    upper_msec = (gdouble) (upper_min==0 && upper_sec==0) ? msec : 1000;

    GTK_ADJUSTMENT(player->sbctx.adj_min)->upper  = upper_min;
    GTK_ADJUSTMENT(player->sbctx.adj_sec)->upper  = upper_sec;
    GTK_ADJUSTMENT(player->sbctx.adj_msec)->upper = upper_msec;
}


static void cb_scale(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

#ifdef _DEBUG
    printf("cb_scale: called\n");
#endif
    ___set_spin_buttons_value(sbctx,
                              gtk_adjustment_get_value(sbctx->adj_scale));
}


static gdouble scale_set_value_from_buttons(spin_button_ctx_t *sbctx)
{
    gdouble sum = 0.0;

    sum += gtk_adjustment_get_value(sbctx->adj_min)   * 60.0 * 1000.0;
    sum += gtk_adjustment_get_value(sbctx->adj_sec)          * 1000.0;
    sum += gtk_adjustment_get_value(sbctx->adj_msec);
    gtk_adjustment_set_value(sbctx->adj_scale, sum);
    return sum;
}


static void cb_spin_msec(GtkWidget *widget, gpointer data)
{
    int msec_value;
    int sec_value;
    int min_value;
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

#ifdef _DEBUG
    printf("cb_spin_msec: called\n");
#endif
    min_value  = (int) gtk_adjustment_get_value(sbctx->adj_min);
    sec_value  = (int) gtk_adjustment_get_value(sbctx->adj_sec);
    msec_value = (int) gtk_adjustment_get_value(sbctx->adj_msec);
    if (min_value == 0 && sec_value == 0 && msec_value < 0) {
        gtk_adjustment_set_value(sbctx->adj_msec, 0.0);
    }
    else if (min_value == sbctx->adj_min->upper &&
             sec_value >= 59 && msec_value > 999)
    {
        gtk_adjustment_set_value(sbctx->adj_msec, 999.0);
    }
    else if (msec_value < 0) {
        gtk_adjustment_set_value(sbctx->adj_msec, 999.0);
        gtk_adjustment_set_value(sbctx->adj_sec, sec_value - 1.0);
    }
    else if (msec_value > 999) {
        gtk_adjustment_set_value(sbctx->adj_msec, 0.0);
        gtk_adjustment_set_value(sbctx->adj_sec, sec_value + 1.0);
    }
    scale_set_value_from_buttons(sbctx);
}


static void cb_spin_sec(GtkWidget *widget, gpointer data)
{
    int sec_value;
    int min_value;
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

#ifdef _DEBUG
    printf("cb_spin_sec: called\n");
#endif
    min_value = (int) gtk_adjustment_get_value(sbctx->adj_min);
    sec_value = (int) gtk_adjustment_get_value(sbctx->adj_sec);
    if (min_value == 0 && sec_value < 0) {
        gtk_adjustment_set_value(sbctx->adj_sec, 0.0);
    }
    else if (min_value == sbctx->adj_min->upper && sec_value > 59) {
        gtk_adjustment_set_value(sbctx->adj_sec, 59.0);
    }
    else if (sec_value < 0) {
        gtk_adjustment_set_value(sbctx->adj_sec, 59.0);
        gtk_adjustment_set_value(sbctx->adj_min, min_value-1);
    }
    else if (sec_value > 59) {
        gtk_adjustment_set_value(sbctx->adj_sec, 0.0);
        gtk_adjustment_set_value(sbctx->adj_min, min_value+1);
    }
    scale_set_value_from_buttons(sbctx);
}


static void cb_spin_min(GtkWidget *widget, gpointer data)
{
    scale_set_value_from_buttons((spin_button_ctx_t *) data);
}


static void cb_block_scale(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

    g_signal_handler_block(sbctx->scale, sbctx->hscale);
}


static void cb_unblock_scale(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

    g_signal_handler_unblock(sbctx->scale, sbctx->hscale);
}


static void cb_block_spinners(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

    g_signal_handler_block(sbctx->spin_msec, sbctx->hmsec);
    g_signal_handler_block(sbctx->spin_sec,  sbctx->hsec);
    g_signal_handler_block(sbctx->spin_min,  sbctx->hmin);
}


static void cb_unblock_spinners(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

    g_signal_handler_unblock(sbctx->spin_msec, sbctx->hmsec);
    g_signal_handler_unblock(sbctx->spin_sec,  sbctx->hsec);
    g_signal_handler_unblock(sbctx->spin_min,  sbctx->hmin);
}


void set_block_pcmview_flag(spin_button_ctx_t *sbctx, gulong value)
{
    sbctx->block_pcmview_seek = value;
}


gulong get_block_pcmview_flag(spin_button_ctx_t *sbctx)
{
    return sbctx->block_pcmview_seek;
}


playback_control_t *
    __construct_playback_control(char *labelstr, long sec, long usec)
{
    GtkWidget          *label;
    GtkWidget          *box_recbuttons;
    GtkWidget          *box_buttons;
    GtkWidget          *box_label;
    GtkWidget          *box_spin;
    GtkWidget          *button;
    GtkWidget          *imgbox;
    GtkWidget          *box;

    GtkObject          *adj_msec;
    GtkObject          *adj_sec;
    GtkObject          *adj_min;
    GtkObject          *adj_scale;
    GtkWidget          *scale;
    GtkWidget          *spin_msec;
    GtkWidget          *spin_sec;
    GtkWidget          *spin_min;
    GtkSettings        *settings;
    GtkRequisition     button_size;
    playback_control_t *player;

    settings         = gtk_settings_get_default();
    player           = (playback_control_t *)
                           calloc(1, sizeof(playback_control_t));
    box              = gtk_vbox_new(FALSE, 0);
    player->playctrl = box;
    box_buttons      = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(box_buttons);

    box_label = gtk_hbox_new(FALSE, 5);
    if (labelstr) {
        /*
         * start label
         */
        gtk_widget_show(box_label);

        label = gtk_label_new(labelstr);
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(box_label), label, FALSE, FALSE, 3);
    }

    gtk_box_pack_start(GTK_BOX(box), box_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), box_buttons, FALSE, FALSE, 0);
    
    /*
     * Time spin boxes
     */
    box_spin = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_spin);

    adj_msec  = gtk_adjustment_new(0.0, -1.0, 1000.0, 1.0, 1.0, 1.0);
    adj_sec   = gtk_adjustment_new(0.0, -1.0, 60.0,   1.0, 1.0, 1.0);
    adj_min   = gtk_adjustment_new(0.0, 0.0,  99.0,   1.0, 1.0, 1.0);
    adj_scale = gtk_adjustment_new(0.0, 0.0,  100.0*60*1000, 5.0, 5.0, 1.0);

    spin_min = gtk_spin_button_new(GTK_ADJUSTMENT(adj_min), 1.0, 0);
    gtk_widget_show(spin_min);

    spin_sec = gtk_spin_button_new(GTK_ADJUSTMENT(adj_sec), 1.0, 0);
    gtk_widget_show(spin_sec);

    spin_msec = gtk_spin_button_new(GTK_ADJUSTMENT(adj_msec), 1.0, 0);
    gtk_widget_show(spin_msec);

    gtk_box_pack_start(GTK_BOX(box_spin), spin_min,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_spin), spin_sec,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_spin), spin_msec, FALSE, FALSE, 0);

    box_recbuttons   = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(box_recbuttons);
    gtk_box_pack_start(GTK_BOX(box), box_recbuttons, FALSE, FALSE, 5);

    /*
     * Record start "green" button
     */
    button = gtk_button_new();
    player->button_recstart = button;
    imgbox = xpm_pixmap_box(settings, IMAGE_RECORD_START);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_widget_show(button);
    gtk_widget_show(imgbox);
    gtk_box_pack_start(GTK_BOX(box_recbuttons), button, FALSE, FALSE, 0);

    /*
     * Record stop "red" button
     */
    button = gtk_button_new();
    player->button_recstop = button;
    imgbox = xpm_pixmap_box(settings, IMAGE_RECORD_STOP);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_widget_show(button);
    gtk_widget_show(imgbox);
    gtk_widget_size_request(button, &button_size);
    gtk_box_pack_start(GTK_BOX(box_recbuttons), button,
                               FALSE, FALSE, button_size.width+5);

    scale = gtk_hscale_new(GTK_ADJUSTMENT(adj_scale));
    gtk_widget_show(scale);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
    player->adj_scale = adj_scale;

    player->sbctx.spin_min  = GTK_SPIN_BUTTON(spin_min);
    player->sbctx.spin_sec  = GTK_SPIN_BUTTON(spin_sec);
    player->sbctx.spin_msec = GTK_SPIN_BUTTON(spin_msec);
    player->sbctx.scale     = GTK_SCALE(scale);
    player->sbctx.adj_min   = GTK_ADJUSTMENT(adj_min);
    player->sbctx.adj_sec   = GTK_ADJUSTMENT(adj_sec);
    player->sbctx.adj_msec  = GTK_ADJUSTMENT(adj_msec);
    player->sbctx.adj_scale = GTK_ADJUSTMENT(adj_scale);
    player->playtime_adj    = adj_scale;
    ___set_scale_spin_buttons_range_from_time(player, sec, usec/1000);
    gtk_box_pack_end(GTK_BOX(box), scale, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(spin_msec), "value-changed",
                     G_CALLBACK(cb_block_scale), &player->sbctx);
    player->sbctx.hmsec = g_signal_connect(G_OBJECT(spin_msec), "value-changed",
                     G_CALLBACK(cb_spin_msec), &player->sbctx);
    g_signal_connect(G_OBJECT(spin_msec), "value-changed",
                     G_CALLBACK(cb_unblock_scale), &player->sbctx);

    g_signal_connect(G_OBJECT(spin_sec), "value-changed",
                     G_CALLBACK(cb_block_scale), &player->sbctx);
    player->sbctx.hsec = g_signal_connect(G_OBJECT(spin_sec), "value-changed",
                     G_CALLBACK(cb_spin_sec), &player->sbctx);
    g_signal_connect(G_OBJECT(spin_sec), "value-changed",
                     G_CALLBACK(cb_unblock_scale), &player->sbctx);

    g_signal_connect(G_OBJECT(spin_min), "value-changed",
                     G_CALLBACK(cb_block_scale), &player->sbctx);
    player->sbctx.hmin = g_signal_connect(G_OBJECT(spin_min), "value-changed",
                     G_CALLBACK(cb_spin_min), &player->sbctx);
    g_signal_connect(G_OBJECT(spin_min), "value-changed",
                     G_CALLBACK(cb_unblock_scale), &player->sbctx);

    g_signal_connect(G_OBJECT(scale), "value-changed",
                     G_CALLBACK(cb_block_spinners), &player->sbctx);
    player->sbctx.hscale = g_signal_connect(G_OBJECT(scale), "value-changed",
                     G_CALLBACK(cb_scale), &player->sbctx);
    g_signal_connect(G_OBJECT(scale), "value-changed",
                     G_CALLBACK(cb_unblock_spinners), &player->sbctx);

    /*
     * Before Play button
     */
    button = gtk_toggle_button_new();
    player->button_playto = button;
    imgbox = xpm_pixmap_box(settings, IMAGE_PLAYTO);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_widget_show(button);
    gtk_widget_show(imgbox);
    gtk_box_pack_start(GTK_BOX(box_buttons), button, FALSE, FALSE, 0);

    /*
     * Pause button.  Displaying this item is configurable from the
     * options menu.
     */
    button = gtk_toggle_button_new();
    player->button_pause = button;
    imgbox = xpm_pixmap_box(settings, IMAGE_PAUSE);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_widget_show(imgbox);
    gtk_box_pack_start(GTK_BOX(box_buttons), button, FALSE, FALSE, 0);

    /*
     * Stop button.  Displaying this item is configurable from the
     * options menu.
     */
    button = gtk_toggle_button_new();
    player->button_stop = button;
    imgbox = xpm_pixmap_box(settings, IMAGE_STOP);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_widget_show(imgbox);
    gtk_box_pack_start(GTK_BOX(box_buttons), button, FALSE, FALSE, 0);

    /*
     * Play button
     */
    button = gtk_toggle_button_new();
    player->button_play = button;
    imgbox = xpm_pixmap_box(settings, IMAGE_PLAY);
    gtk_container_add(GTK_CONTAINER(button), imgbox);
    gtk_widget_show(button);
    gtk_widget_show(imgbox);
    gtk_box_pack_start(GTK_BOX(box_buttons), button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box_buttons), box_spin, FALSE, FALSE, 0);
    return player;
}


#ifdef _UNIT_TEST


int main(int argc, char *argv[])
{
    playback_control_t *player;
    GtkWidget *window;
    GtkWidget *vbox;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_show(window);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    player = __construct_playback_control("Test player", 45, 0);
    gtk_widget_show(player->playctrl);

    gtk_box_pack_start(GTK_BOX(vbox), player->playctrl, FALSE, 0, 0);
    gtk_main();
    return 0;
}
#endif
