#include <gtk/gtk.h>
#include <gtk/gtkenums.h>
#include <gtk/gtkrange.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../portability.h"

typedef struct _time_spinbutton_ctx {
    int       ignore;
    GtkWidget *minute;
    GtkWidget *second;
    GtkWidget *millisecond;
    GtkObject *minute_adj;
    GtkObject *second_adj;
    GtkObject *millisecond_adj;

    gdouble   minute_prev;
    gdouble   second_prev;
    gdouble   millisecond_prev;
} time_spinbutton_ctx;


typedef struct _idle_ctx {
    GtkWidget *label_elapsed;
    GtkWidget *label_start;
    GtkWidget *label_trt;
    GtkWidget *entry;
    GtkWidget *open_file_toggle;
    GtkWidget *scrollbar;
    GtkWidget *play_toggle;
    GtkWidget *playto_toggle;
    GtkWidget *pause_toggle;
    GtkWidget *record;
    GtkTooltips *tooltips;
    GtkObject *adj;
    gint      idle_tag;
    void      *playctx;
    char      *play_file;
    int       play_toggle_ignore;
    int       adjustment_ignore;

    int      stime_sec;
    int      stime_usec;
    int      elapsed_sec;
    int      elapsed_usec;
    time_spinbutton_ctx sbctx;
} idle_ctx;


gint cb_delete(GtkWidget *widget,
               GdkEvent  *event,
               gpointer   data )
{
    gtk_main_quit();
    return FALSE;
}


void cb_spin_minute(GtkWidget *widget, gpointer *data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    gdouble  minute;
    int      delta;
static int __depth=0;

    if (idlectx->sbctx.ignore) {
printf("cb_spin_minute: ignore set, quitting...\n");
        return;
    }
printf("cb_spin_minute: in %d\n", __depth++);
    minute = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    delta  = (int) (minute - idlectx->sbctx.minute_prev);
    if (idlectx->sbctx.minute_prev + delta <= 0.0) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0.0);
        idlectx->sbctx.minute_prev = 0.0;
printf("cb_spin_minute: out %d\n", --__depth);
        return;
    }

    idlectx->sbctx.minute_prev = minute;
    idlectx->stime_sec        += (int) delta * 60;
printf("cb_spin_minute: out %d\n", --__depth);
}


void cb_spin_second(GtkWidget *widget, gpointer *data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    gdouble  second;
    gdouble  delta;
static int __depth = 0;

    if (idlectx->sbctx.ignore) {
        idlectx->sbctx.ignore = 0;
printf("cb_spin_second: ignore set, quitting...\n");
        return;
    }

printf("cb_spin_second: in %d\n", __depth++);
    second = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    delta  = second - idlectx->sbctx.second_prev;

    /*
     * Second cannot spin below zero when minute is already zero
     */
    if (second <= 0.0 && idlectx->sbctx.minute_prev <= 0.0) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0.0);
        idlectx->sbctx.second_prev = second;
printf("cb_spin_second: out %d\n", --__depth);
        return;
    }

    /*
     * Increment minute by one, and set second back to zero.
     */
    if (second >= 60.0) {
        gtk_spin_button_spin(GTK_SPIN_BUTTON(idlectx->sbctx.minute),
                             GTK_SPIN_STEP_FORWARD, 1.0);
        idlectx->sbctx.second_prev = 0.0;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0.0);
    }
    else if (second < 0.0) {
        gtk_spin_button_spin(GTK_SPIN_BUTTON(idlectx->sbctx.minute),
                             GTK_SPIN_STEP_BACKWARD, 1.0);
        second = 60.0 + delta;
        idlectx->sbctx.second_prev = second;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), second);
    }
    else {
        idlectx->sbctx.second_prev = second;
    }
    idlectx->stime_sec += (int) delta;
printf("cb_spin_second: out %d\n", --__depth);
}


void cb_spin_millisecond(GtkWidget *widget, gpointer *data)
{
    idle_ctx *idlectx = (idle_ctx *) data;
    gdouble  millisecond;
    gdouble  delta;
static int __depth = 0;

    if (idlectx->sbctx.ignore) {
printf("cb_spin_millisecond: ignore set, quitting...\n");
        return;
    }

printf("cb_spin_millisecond: in %d\n", __depth++);
    millisecond = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    delta       = millisecond - idlectx->sbctx.millisecond_prev;
    if (idlectx->sbctx.millisecond_prev + delta <= 0.0 &&
        idlectx->sbctx.second_prev <= 0.0 &&
        idlectx->sbctx.minute_prev <= 0.0)
    {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0.0);
        idlectx->sbctx.millisecond_prev = 0.0;
printf("cb_spin_millisecond: out %d\n", --__depth);
        return;
    }

    if (millisecond >= 1000.0) {
        gtk_spin_button_spin(GTK_SPIN_BUTTON(idlectx->sbctx.second),
                             GTK_SPIN_STEP_FORWARD, 1.0);
        idlectx->sbctx.millisecond_prev = 0.0;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), 0.0);
    }
    else if (millisecond < 0.0) {
        gtk_spin_button_spin(GTK_SPIN_BUTTON(idlectx->sbctx.second),
                             GTK_SPIN_STEP_BACKWARD, 1.0);
        millisecond = 1000.0 + delta;
        idlectx->sbctx.millisecond_prev = millisecond;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), millisecond);
    }
    else {
        idlectx->sbctx.millisecond_prev = millisecond;
    }
    idlectx->stime_usec += (int) delta * 1000;
printf("cb_spin_millisecond: out %d\n", --__depth);
}


void ___spin_button_set_value(idle_ctx *idlectx)
{
    long min  = idlectx->stime_sec / 60;
    long sec  = idlectx->stime_sec % 60;
    long msec = idlectx->stime_usec;

    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(idlectx->sbctx.minute), min);

    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(idlectx->sbctx.second), sec);

    gtk_spin_button_set_value(
        GTK_SPIN_BUTTON(idlectx->sbctx.millisecond), msec);

    idlectx->sbctx.minute_prev = min;
    idlectx->sbctx.second_prev = sec;
    idlectx->sbctx.millisecond_prev = msec;
}



int main(int argc, char *argv[]) 
{

    GtkWidget *box_spin;
    GtkWidget *window;
    GtkWidget *spin;
    GtkWidget *box;
    GtkWidget *button;
    GtkObject *spinadj;
    idle_ctx idlectx;
    memset(&idlectx, 0, sizeof(idlectx));

    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(window, 220, 180);
    gtk_window_set_title(GTK_WINDOW(window), "xmpgedit 0.01");
    g_signal_connect(G_OBJECT(window), "delete_event",
                     G_CALLBACK(cb_delete), &idlectx);

    box = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(box);

    button = gtk_button_new_with_label("Enter Time");
    gtk_widget_show(button);

    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    /*
     * Time spin boxes
     */
    box_spin = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(box_spin);

    spinadj = gtk_adjustment_new(0.0, 0.0, 9999.0, 1.0, 10.0, 0.0);
    idlectx.sbctx.minute_adj = spinadj;
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(spinadj), 10.0, 0);
    gtk_widget_show(spin);
    gtk_box_pack_start(GTK_BOX(box_spin), spin, FALSE, FALSE, 0);
    idlectx.sbctx.minute = spin;
    g_signal_connect(G_OBJECT(spin), "value_changed",
                     G_CALLBACK(cb_spin_minute), &idlectx);


    spinadj = gtk_adjustment_new(0.0, -1.0, 61.0, 1.0, 10.0, 0.0);
    idlectx.sbctx.second_adj = spinadj;
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(spinadj), 10.0, 0);
    gtk_widget_show(spin);
    gtk_box_pack_start(GTK_BOX(box_spin), spin, FALSE, FALSE, 0);
    idlectx.sbctx.second = spin;
    g_signal_connect(G_OBJECT(spin), "value_changed",
                     G_CALLBACK(cb_spin_second), &idlectx);
    
    spinadj = gtk_adjustment_new(0.0, -1.0, 1001.0, 5.0, 20.0, 0.0);
    idlectx.sbctx.millisecond_adj = spinadj;
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(spinadj), 10.0, 0);
    gtk_widget_show(spin);
    gtk_box_pack_start(GTK_BOX(box_spin), spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), box_spin, FALSE, FALSE, 0);
    idlectx.sbctx.millisecond = spin;
    g_signal_connect(G_OBJECT(spin), "value_changed",
                     G_CALLBACK(cb_spin_millisecond), &idlectx);
    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_widget_show(window);
    idlectx.stime_sec = 666;
    
#if 1
    idlectx.sbctx.ignore = 1;
    ___spin_button_set_value(&idlectx);
    idlectx.sbctx.ignore = 0;
#endif
    gtk_main();
    
    return 0;
}


_MAIN(main)
