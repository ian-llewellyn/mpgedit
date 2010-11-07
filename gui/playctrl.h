#ifndef _PLAYCTRL_H_
#define _PLAYCTRL_H_

#include <gtk/gtk.h>

#define IMAGE_PAUSE        "pause.xpm"
#define IMAGE_PLAY         "play.xpm"
#define IMAGE_PLAYTO       "next_t.xpm"
#define IMAGE_RECORD       "record.xpm"
#define IMAGE_RECORD_START "record_green.xpm"
#define IMAGE_RECORD_STOP  "record_red.xpm"
#define IMAGE_STOP         "stop.xpm"
#define IMAGE_EJECT        "eject.xpm"
#define IMAGE_CLOSE        "close.xpm"


typedef struct _spin_button_ctx_t
{
    GtkScale      *scale;
    GtkWidget     *time_scrollbar;
    GtkSpinButton *spin_msec;
    GtkSpinButton *spin_sec;
    GtkSpinButton *spin_min;

    GtkAdjustment *adj_msec;
    GtkAdjustment *adj_sec;
    GtkAdjustment *adj_min;
    GtkAdjustment *adj_scale;

    gulong        hmsec;
    gulong        hsec;
    gulong        hmin;
    gulong        hscale;
    gulong        hadj_scale;

    gulong        block_pcmview_seek;
} spin_button_ctx_t;

typedef struct _playback_control_t {
    GtkWidget *playctrl;
    GtkObject *adj_scale;
    GtkObject *playtime_adj;
    GtkWidget *button_playto;
    GtkWidget *button_pause;
    GtkWidget *button_stop;
    GtkWidget *button_play;
    GtkWidget *button_recstart;
    GtkWidget *button_recstop;

    GtkWidget *label_start;
    spin_button_ctx_t sbctx; 


} playback_control_t;



GtkWidget *xpm_pixmap(GtkSettings *settings, gchar *xpm_filename);

GtkWidget *xpm_pixmap_box(GtkSettings *settings, gchar *xpm_filename);

playback_control_t *__construct_playback_control(
                                  char *labelstr,
                                  long sec,
                                  long usec);

void ___set_scale_spin_buttons_range_from_time(playback_control_t *player,
                                               long              sec,
                                               long              msec);

void ___spin_button_set_value(spin_button_ctx_t *sbctx, int insec, int inmsec);

gdouble ___scale_set_value(spin_button_ctx_t *sbctx, long sec, long msec);


void label_set_text(GtkWidget *widget, char *msg, ...);

void set_block_pcmview_flag(spin_button_ctx_t *sbctx, gulong value);

gulong get_block_pcmview_flag(spin_button_ctx_t *sbctx);



#endif
