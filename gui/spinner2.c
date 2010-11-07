/*
 * Example of a time widget with a scale and three
 * spin buttons.  All of these controls interact
 * in a rational manner.  This technique will replace
 * the mess currently in xmpgedit.
 */
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

#include "playctrl.h"

gdouble scale_set_value_from_buttons(spin_button_ctx_t *sbctx)
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


void set_scale_value(spin_button_ctx_t *sbctx, gdouble value)
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


static void cb_scale(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;

#ifdef _DEBUG
    printf("cb_scale: called\n");
#endif
    set_scale_value(sbctx, gtk_adjustment_get_value(sbctx->adj_scale));
}


/*
 * This is the user's callback. All of the other callbacks are internal
 * to the time scale widget.
 */
void cb_user_scale(GtkWidget *widget, gpointer data)
{
    spin_button_ctx_t *sbctx = (spin_button_ctx_t *) data;
    int min_value;
    int sec_value;
    int msec_value;

    min_value  = (int) gtk_adjustment_get_value(sbctx->adj_min);
    sec_value  = (int) gtk_adjustment_get_value(sbctx->adj_sec);
    msec_value = (int) gtk_adjustment_get_value(sbctx->adj_msec);
    printf("cb_user_scale: %d:%02d.%03d\n", min_value, sec_value, msec_value);
}


static void cb_spin_min(GtkWidget *widget, gpointer data)
{
#ifdef _DEBUG
    printf("cb_spin_min: called\n");
#endif
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


spin_button_ctx_t *time_scrollbar_new(void)
{
    GtkObject *adj_msec;
    GtkObject *adj_sec;
    GtkObject *adj_min;
    GtkObject *adj_scale;
    GtkWidget *scale;
    GtkWidget *spin_msec;
    GtkWidget *spin_sec;
    GtkWidget *spin_min;
    GtkWidget *hbox;
    GtkWidget *vbox;
    spin_button_ctx_t *sbctx;

    sbctx = (spin_button_ctx_t *) calloc(1, sizeof(spin_button_ctx_t));

    vbox = gtk_vbox_new(FALSE, 4);
    sbctx->time_scrollbar = vbox;

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_widget_show(hbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
   
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
    
    scale = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
    gtk_widget_show(scale);
    gtk_range_set_adjustment(GTK_RANGE(scale), GTK_ADJUSTMENT(adj_scale));
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DELAYED);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);

    gtk_box_pack_start(GTK_BOX(hbox), spin_min,  FALSE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), spin_sec,  FALSE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), spin_msec, FALSE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), scale,     FALSE, TRUE, 2);

    sbctx->spin_min  = GTK_SPIN_BUTTON(spin_min);
    sbctx->spin_sec  = GTK_SPIN_BUTTON(spin_sec);
    sbctx->spin_msec = GTK_SPIN_BUTTON(spin_msec);
    sbctx->scale     = GTK_SCALE(scale);
    sbctx->adj_min   = GTK_ADJUSTMENT(adj_min);
    sbctx->adj_sec   = GTK_ADJUSTMENT(adj_sec);
    sbctx->adj_msec  = GTK_ADJUSTMENT(adj_msec);
    sbctx->adj_scale = GTK_ADJUSTMENT(adj_scale);

    g_signal_connect(G_OBJECT(spin_msec), "value-changed", 
                     G_CALLBACK(cb_block_scale), sbctx);
    sbctx->hmsec = g_signal_connect(G_OBJECT(spin_msec), "value-changed", 
                     G_CALLBACK(cb_spin_msec), sbctx);
    g_signal_connect(G_OBJECT(spin_msec), "value-changed", 
                     G_CALLBACK(cb_unblock_scale), sbctx);

    g_signal_connect(G_OBJECT(spin_sec), "value-changed", 
                     G_CALLBACK(cb_block_scale), sbctx);
    sbctx->hsec = g_signal_connect(G_OBJECT(spin_sec), "value-changed", 
                     G_CALLBACK(cb_spin_sec), sbctx);
    g_signal_connect(G_OBJECT(spin_sec), "value-changed", 
                     G_CALLBACK(cb_unblock_scale), sbctx);

    g_signal_connect(G_OBJECT(spin_min), "value-changed", 
                     G_CALLBACK(cb_block_scale), sbctx);
    sbctx->hmin = g_signal_connect(G_OBJECT(spin_min), "value-changed", 
                     G_CALLBACK(cb_spin_min), sbctx);
    g_signal_connect(G_OBJECT(spin_min), "value-changed", 
                     G_CALLBACK(cb_unblock_scale), sbctx);

    g_signal_connect(G_OBJECT(scale), "value-changed", 
                     G_CALLBACK(cb_block_spinners), sbctx);
    sbctx->hscale = g_signal_connect(G_OBJECT(scale), "value-changed", 
                     G_CALLBACK(cb_scale), sbctx);
    g_signal_connect(G_OBJECT(scale), "value-changed", 
                     G_CALLBACK(cb_unblock_spinners), sbctx);
    return sbctx;
}


int main(int argc, char *argv[])
{
    GtkWidget         *window;
    spin_button_ctx_t *sbctx;

    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sbctx  = time_scrollbar_new();
    g_signal_connect(G_OBJECT(sbctx->scale), "value-changed", 
                     G_CALLBACK(cb_user_scale), sbctx);

    gtk_container_add(GTK_CONTAINER(window), sbctx->time_scrollbar);
    gtk_widget_show(sbctx->time_scrollbar);
    gtk_widget_show(window);
    gtk_main();
    return 0;
}
