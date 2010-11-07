
/* 
 * pcm volume levels visualization
 *
 * Copyright (C) 2004-2008 Adam Bernstein. All Rights Reserved.
 * 
 * Cribbed "scribble-simple" code from GTK examples as a starting point.
 * GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <pcmlevel.h>
#include "pcmview.h"
#include "portability.h"


#define PCM_MAX_VALUE 65536

#define WINSIZE_X 400
#define WINSIZE_Y 150
#define TIC_LINE_LEN 15 
#define TIC_TEXT_OFFSET 10
#define TIC_BOX_HEIGHT (15+TIC_TEXT_OFFSET)
#define RECT_WIDTH 1

#define REDRAW_GC(w) (w->style->light_gc[3])

typedef struct _pcmview_scroll_idlectx_t {
    mpgedit_pcmview_ctx *pcmviewctx;
    int                 sec_incr;
} pcmview_scroll_idlectx_t;


#define GLOBAL_CTX(d) ((mpgedit_pcmview_ctx *) d)
static void redraw_pcmdata_values(gpointer data);
static int _mpgedit_pcmview_get_data_xpos(
              struct _mpgedit_pcmview_ctx *ctx,
              int  *pcmvalue,
              long *pcmsec,
              long *pcmmsec);
static int _mpgedit_pcmview_sec_to_xpos(
              struct _mpgedit_pcmview_ctx *ctx,
              long lsec,
              long lmsec);
static void _mpgedit_pcmview_seek_sec(mpgedit_pcmview_ctx *ctx, long sec);
static long pcmview_seek_xpos(mpgedit_pcmview_ctx *ctx, int xpos);

/* Create a new backing pixmap of the appropriate size */
static gint configure_event( GtkWidget         *widget,
                             GdkEventConfigure *event,
                             gpointer data)
{
  mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);

  if (gctx->pixmap) {
    g_object_unref (gctx->pixmap);
  }
  gctx->pixmap = gdk_pixmap_new (widget->window,
			   widget->allocation.width,
			   widget->allocation.height,
			   -1);
  gdk_draw_rectangle (gctx->pixmap,
		      widget->style->white_gc,
		      TRUE,
		      0, 0,
		      widget->allocation.width,
		      widget->allocation.height);

  redraw_pcmdata_values(data);

  return TRUE;
}

/* Redraw the screen from the backing pixmap */
static gint expose_event( GtkWidget      *widget,
                          GdkEventExpose *event,
                          gpointer data)
{
  mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);

  gdk_draw_drawable (widget->window,
		     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		     gctx->pixmap,
		     event->area.x, event->area.y,
		     event->area.x, event->area.y,
		     event->area.width, event->area.height);

  return FALSE;
}

#if 0
/* Draw a rectangle on the screen */
static void draw_brush( GtkWidget *widget,
                        gpointer data,
                        gdouble    x,
                        gdouble    y)
{
  GdkRectangle update_rect;
  mpgedit_pcmview_ctx* gctx = GLOBAL_CTX(data);

  update_rect.x = x - 1;
  update_rect.y = y - 1;
  update_rect.width = 2;
  update_rect.height = 2;
  gdk_draw_rectangle (gctx->pixmap,
		      widget->style->black_gc,
		      TRUE,
		      update_rect.x, update_rect.y,
		      update_rect.width, update_rect.height);

  gtk_widget_queue_draw_area (widget, 
		      update_rect.x, update_rect.y,
		      update_rect.width, update_rect.height);
}
#endif


static void draw_cursor_line(GtkWidget           *widget,
                             mpgedit_pcmview_ctx *gctx,
                             int                 percent,
                             gdouble             x)
{
  int height = gctx->drawing_area->allocation.height * percent/100;

  if (!gctx->pixmap_shadow) {
    return;
  }
  if (height < 0) {
    height = -height;

    gdk_draw_drawable(gctx->pixmap,
                      gctx->drawing_area->style->fg_gc[
                          GTK_WIDGET_STATE(GTK_WIDGET(gctx->drawing_area))],
                      gctx->pixmap_shadow,
                      0, 0, 
                      gctx->drawing_area->allocation.width, height,
                      0, 0);
  /* light_gc[3] is good */
    gdk_draw_line(gctx->pixmap, widget->style->light_gc[3], 
                  (gint) x,  height,
                  (gint) x,  0);
    gtk_widget_queue_draw_area(widget,
                  gctx->drawing_area->allocation.width, height,
                  0,  0);
  }
  else {

    gdk_draw_drawable(gctx->pixmap,
                      gctx->drawing_area->style->fg_gc[
                          GTK_WIDGET_STATE(GTK_WIDGET(gctx->drawing_area))],
                      gctx->pixmap_shadow,
                      0, 0, 
                      0, 0,
                      gctx->drawing_area->allocation.width,
                      height);
  /* light_gc[3] is good */
    gdk_draw_line(gctx->pixmap, widget->style->light_gc[3], 
                  (gint) x,  0,
                  (gint) x,  height);
    gtk_widget_queue_draw_area(widget,
                  0,  0,
                  gctx->drawing_area->allocation.width,
                  height);
  }
}


static void draw_line(GtkWidget *widget,
                      gpointer data,
                      int percent,
                      gdouble x,
                      gdouble y)
{
    mpgedit_pcmview_ctx* gctx = GLOBAL_CTX(data);

    D_printf(("draw_line called: x=%f y=%f pct=%d\n", x, y, percent));

    draw_cursor_line(widget, gctx, percent, x);
    if (gctx->click_callback) {
        gctx->click_callback(widget, data, gctx->button_click, x, y);
    }
}


static void clear_drawing_area(mpgedit_pcmview_ctx *gctx)
{

    /*
     * Clear the drawing area 
     */
    gdk_draw_rectangle(gctx->pixmap,
                       gctx->drawing_area->style->white_gc,
                       TRUE,
                       0, 0,
                       gctx->drawing_area->allocation.width,
                       gctx->drawing_area->allocation.height);

    /*
     * Draw the current screen state onto the visable window
     */
    gtk_widget_queue_draw_area(gctx->drawing_area,
                  0, 0, 
                  gctx->drawing_area->allocation.width,
                  gctx->drawing_area->allocation.height);
}


/*
 * Draw a segment of pcmdata values, starting at xpos and ending at
 * ypos, using the specified graphics context.
 */
static void draw_pcmdata_values_segment(mpgedit_pcmview_ctx *gctx)
{
    int  draw_count;
    int  read_sts;
    int  pcm_value;
    long pcm_sec, pcm_msec;
    int  window_height = gctx->drawing_area->allocation.height;
    int  scale = PCM_MAX_VALUE / (window_height-TIC_BOX_HEIGHT);

    D_printf(("draw_pcmdata_values_segment: gc=%p (b/e)=%d/%d\n", 
        gctx->redraw_gc,
        gctx->redraw_b, gctx->redraw_e));

    draw_count   = gctx->redraw_b;

    /* Seek to "eventx" position, and return data */
    read_sts = pcmview_seek_xpos(gctx, gctx->redraw_b);
    if (read_sts == -1) {
        return;
    }
    read_sts  = mpgedit_pcmlevel_read_entry(gctx->pcmfp, &pcm_value,
                                            &pcm_sec, &pcm_msec);
    while (draw_count < gctx->redraw_e && read_sts) {
        /*
         * Draw the PCM data volume level
         */
        gdk_draw_line(gctx->pixmap, gctx->redraw_gc,
                      draw_count, window_height,
                      draw_count, window_height - (pcm_value / scale));
        draw_count  += RECT_WIDTH;
        read_sts = mpgedit_pcmlevel_read_entry(gctx->pcmfp, &pcm_value,
                                               &pcm_sec, &pcm_msec);
    }
    if (!read_sts) {
        gctx->scroll_stop = 1;
    }
    gctx->pcm_cursorvalue = pcm_value;

    gtk_widget_queue_draw_area(gctx->drawing_area, 0, 0, 
                               gctx->drawing_area->allocation.width,
                               gctx->drawing_area->allocation.height);
}


static void draw_pcmdata_values(mpgedit_pcmview_ctx *gctx)
{
    int          window_height;
    int          scale;
    int          pcm_value;
    long         pcm_sec, pcm_msec;
    long         pcm_seclast = 0, pcm_mseclast = 0;
    int          draw_threshold;
    int          read_sts;
    int          total_secs = 0;
    char         str[128];
    int          draw_count = 0;
    long         prev_save;
    long         read_pcm_sec;
    long         read_pcm_msec;

    if (!gctx || !gctx->pcmfp) {
        return;
    }
    prev_save = mpgedit_pcmlevel_tell(gctx->pcmfp);
    read_sts  = mpgedit_pcmlevel_read_entry(gctx->pcmfp, &pcm_value,
                                            &read_pcm_sec, &read_pcm_msec);
    if (!read_sts) {
        return;
    }

    pcm_sec  = read_pcm_sec  - gctx->pcm_secoffset;
    pcm_msec = read_pcm_msec - gctx->pcm_msecoffset;
    D_printf(("Start pcmsec: %ld:%02ld.%03ld\n", 
              pcm_sec/60, pcm_sec%60, pcm_msec));

    gctx->pcm_secfirst  = pcm_sec;
    gctx->pcm_msecfirst = pcm_msec;

    /*
     * Clear the drawing area 
     */
    gdk_draw_rectangle(gctx->pixmap,
                       gctx->drawing_area->style->white_gc,
                       TRUE,
                       0, 0,
                       gctx->drawing_area->allocation.width,
                       gctx->drawing_area->allocation.height);
    window_height = gctx->drawing_area->allocation.height;

    scale = PCM_MAX_VALUE / (window_height-TIC_BOX_HEIGHT);
    while (draw_count < gctx->drawing_area->allocation.width &&
           read_sts) 
    {
        /*
         * Draw 20 second tics
         */
        if (pcm_sec && ((pcm_sec%20)==0) && pcm_sec>total_secs) {
            total_secs = pcm_sec;
            gdk_draw_line(gctx->pixmap, gctx->drawing_area->style->black_gc,
                          draw_count, 2, draw_count, TIC_LINE_LEN);

            sprintf(str, "%ld:%02ld", pcm_sec/60, pcm_sec%60);
            gdk_draw_text(gctx->pixmap, gctx->font_fixed,
                          gctx->drawing_area->style->black_gc,
                          draw_count>20 ? draw_count-20 : 0, TIC_LINE_LEN+10,
                          str, strlen(str));
        }

        /*
         * Draw the PCM data volume level
         */
        gdk_draw_line(gctx->pixmap, gctx->drawing_area->style->black_gc,
                      draw_count, window_height,
                      draw_count, window_height - (pcm_value / scale));
        draw_count  += RECT_WIDTH;
        pcm_seclast  = pcm_sec;
        pcm_mseclast = pcm_msec;
        read_sts = mpgedit_pcmlevel_read_entry(gctx->pcmfp, &pcm_value,
                                               &read_pcm_sec, &read_pcm_msec);
        pcm_sec  = read_pcm_sec  - gctx->pcm_secoffset;
        pcm_msec = read_pcm_msec - gctx->pcm_msecoffset;
    }

    D_printf(("End pcmsec:  %ld:%02ld.%03ld\n\n",
              pcm_seclast/60, pcm_seclast%60, pcm_mseclast));
    gctx->pcm_seclast  = pcm_seclast;
    gctx->pcm_mseclast = pcm_mseclast;
    
    /*
     * draw_count == drawing_area->allocation.width, except when the amount
     * of data drawn is less than width of the drawing_area.  This happens
     * when the window has been scrolled right past the end of the
     * current file data.  draw_count always has the correct value, as it
     * contains the count of the number of samples plotted.
     */
    gctx->pixel_per_sec =
        (gdouble) draw_count /
        (((gdouble) pcm_seclast + (gdouble) pcm_mseclast/1000.0) -
         ((gdouble) gctx->pcm_secfirst +
          (gdouble) gctx->pcm_msecfirst/1000.0));
    D_printf(("pixel_per_sec=%f\n", gctx->pixel_per_sec));
    draw_threshold = window_height - ((gctx->pcm_avg >> gctx->decibels)/scale);
    gdk_draw_line(gctx->pixmap, gctx->drawing_area->style->white_gc,
                  0, draw_threshold,
                  gctx->drawing_area->allocation.width, draw_threshold);
    /*
     * Store current screen state so can "erase" cursor line
     */
    if (gctx->pixmap_shadow) {
      g_object_unref(gctx->pixmap_shadow);
    }
    gctx->pixmap_shadow = 
        gdk_pixmap_new(GTK_WIDGET(gctx->drawing_area)->window,
                       gctx->drawing_area->allocation.width,
                       gctx->drawing_area->allocation.height, -1);
    gdk_draw_drawable(gctx->pixmap_shadow, 
		      gctx->drawing_area->style->fg_gc[
                          GTK_WIDGET_STATE(GTK_WIDGET(gctx->drawing_area))],
                      gctx->pixmap,
                      0, 0, 
                      0, 0, 
                      gctx->drawing_area->allocation.width,
                      gctx->drawing_area->allocation.height);

    /*
     * Draw the current screen state onto the visable window
     */
    gtk_widget_queue_draw_area(gctx->drawing_area,
                  0, 0, 
                  gctx->drawing_area->allocation.width,
                  gctx->drawing_area->allocation.height);
    return;
}


static void redraw_pcmdata_values(gpointer data)
{
    mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);
    long fpos;

    fpos = mpgedit_pcmlevel_index_get_offset(gctx->pcmfp,
                                             gctx->pcmindex,
                                             gctx->pcm_secfirst);
    if (fpos != -1) {
        mpgedit_pcmlevel_seek(gctx->pcmfp, fpos);
        draw_pcmdata_values(gctx);
    }
}


/*
 * Set viewer position by x percent.  A negative percent value
 * scrolls to the left, positive to the right.
 */
static void pcmdata_set_start(mpgedit_pcmview_ctx *gctx, int sec_adv)
{
    gdouble sec_start;
    gdouble sec_new;
    
    sec_start = gctx->pcm_secfirst + ((gdouble) gctx->pcm_msecfirst) / 1000.0;
    sec_new   = sec_start + (gdouble) sec_adv;

    if (sec_new < 0) {
        sec_new = 0;
    }
    _mpgedit_pcmview_seek_sec(gctx, (long) sec_new);
}



static void next_button_press(GtkWidget *widget, gpointer cbdata)
{
    draw_pcmdata_values(GLOBAL_CTX(cbdata));
}


static void prev_button_press(GtkWidget *widget, gpointer cbdata)
{
    mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(cbdata);
    long                seek_sec;

    seek_sec = gctx->pcm_secfirst + (gctx->pcm_secfirst - gctx->pcm_seclast);
    if (seek_sec < 0) {
        seek_sec = 0;
    }
    _mpgedit_pcmview_seek_sec(gctx, seek_sec);
}


/*
 * You can change gctx->pcmfile, and the next
 * button press will open that file.
 */
static void draw_button_press(GtkWidget *widget,
                               gpointer  cbdata)
{
    int ver;
    int pcmbits;
    int secbits;
    int msecbits;
    int sts;
    
    mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(cbdata);

    D_printf(("draw_button_press: called\n"));

    if (gctx->pcmfp) {
        mpgedit_pcmlevel_close(gctx->pcmfp);
    }

    gctx->pcmfp = mpgedit_pcmlevel_open(gctx->pcmfile, "rb");
    if (!gctx->pcmfp) {
        return;
    }
    sts = mpgedit_pcmlevel_read_header(gctx->pcmfp, &ver, 
                                       &pcmbits, &secbits, &msecbits);
    if (!sts) {
        return;
    }

    if (ver == 1 && pcmbits == 16 && secbits == 22 && msecbits == 10) {
        mpgedit_pcmlevel_read_average(gctx->pcmfp, &gctx->pcm_avg, 
                                      &gctx->pcm_max, &gctx->pcm_min);
        gctx->prev_indx = 0;
        draw_pcmdata_values(gctx);
    }
    if (gctx->pcmindex) {
        mpgedit_pcmlevel_index_free(gctx->pcmindex);
    }
    gctx->pcmindex = mpgedit_pcmlevel_generate_index(gctx->pcmfp);
    return;
}


static void close_viewer(mpgedit_pcmview_ctx *gctx)
{
    if (!gctx->pcmfp || !gctx->pixmap) {
        return;
    }
    clear_drawing_area(gctx);
    mpgedit_pcmlevel_close(gctx->pcmfp);
    gctx->pcmfp = NULL;
    gctx->pcmfile = NULL;
    g_object_unref (gctx->pixmap_shadow);
    gctx->pixmap_shadow = NULL;
}


static gint button_press_event_pre( GtkWidget  *widget,
                                GdkEventButton *event,
                                gpointer data)
{
  mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);
  if (gctx->click_callback_pre) {
    gctx->click_callback_pre(widget, data);
  }
  return FALSE;
}


static gint button_press_event_post( GtkWidget *widget,
                                GdkEventButton *event,
                                gpointer data)
{
  mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);
  if (gctx->click_callback_post) {
    gctx->click_callback_post(widget, data);
  }
  return TRUE;
}


static gint button_press_event( GtkWidget      *widget,
                                GdkEventButton *event,
                                gpointer data)
{
    mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);
    unsigned int        clicked_button;
    int pcmvalue;
    long pcmsec, pcmmsec;

#if 0
    if (event->button == 1 && gctx->pixmap)
        draw_brush (widget, data, event->x, event->y);
#endif

    clicked_button = 1 << (event->button-1);
    if ((clicked_button & gctx->button_mask) && gctx->pixmap) {
        gctx->button_click = event->button;
        gctx->button_state = BUTTON_DOWN;
        gctx->eventx       = (int) event->x;
        gctx->eventy       = (int) event->y;
        gctx->direction    = -1;
        gctx->scroll_sec   = 0;
        gctx->redraw_b     = gctx->eventx;
        gctx->redraw_e     = gctx->eventx;
        gctx->redraw_p     = gctx->eventx;
        _mpgedit_pcmview_get_data_xpos(gctx, &pcmvalue, &pcmsec, &pcmmsec);
        D_printf(("button_press_event: b=%d, (x/y)=%d/%d, v=%d %ld.%ld\n",
                  event->button, gctx->eventx, gctx->eventy,
                  pcmvalue, pcmsec, pcmmsec));
        gctx->pick_xfirst    = gctx->eventx;
        gctx->pick_yfirst    = gctx->eventy;
        gctx->pick_secfirst  = pcmsec;
        gctx->pick_msecfirst = pcmmsec;
        draw_line(widget, data, 100, event->x, event->y);
    }

    return FALSE;
}


static gint button_release_event(GtkWidget      *widget,
                                 GdkEventButton *event,
                                 gpointer       data)
{
    mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);
    unsigned int        clicked_button;
    int                 pcmvalue;
    long                pcmsec, pcmmsec;
    int                 event_delta = 0;

    D_printf(("button_release_event: b=%d, %d %d\n",
              event->button, (int) event->x, (int) event->y));

    clicked_button = 1 << (event->button-1);
    if ((clicked_button & gctx->button_mask) && gctx->pixmap) {
        gctx->button_click = event->button;
        gctx->button_state = BUTTON_UP;
        gctx->scroll_stop  = TRUE;
        gctx->eventx       = (int) event->x;
        gctx->eventy       = (int) event->y;
        _mpgedit_pcmview_get_data_xpos(gctx, &pcmvalue, &pcmsec, &pcmmsec);
        D_printf(("button_release_event: b=%d, (x/y)=%d/%d, v=%d %ld.%ld\n",
                  event->button, gctx->eventx, gctx->eventy,
                  pcmvalue, pcmsec, pcmmsec));
        gctx->pick_seclast  = pcmsec;
        gctx->pick_mseclast = pcmmsec;

        event_delta = gctx->eventx - gctx->pick_xfirst;
        /* Ensure event delta is always positive for evaluation */
        if (event_delta < 0) {
            event_delta = -event_delta;
        }

        /* Picks are less than threshold */
        D_printf(("event_delta=%d\n", event_delta));
        if (event_delta <= PCMVIEW_PICK_THRESHOLD) {
            D_printf(("!!! Skipping callback, too few picks !!!\n"));
            return FALSE;
        }

        /* Ensure times are always in ascending order */
        if ((gctx->pick_secfirst > gctx->pick_seclast) || 
            (gctx->pick_secfirst == gctx->pick_seclast  &&
             gctx->pick_msecfirst > gctx->pick_mseclast)) 
        {
            gctx->pick_seclast   = gctx->pick_secfirst;
            gctx->pick_mseclast  = gctx->pick_msecfirst;

            gctx->pick_secfirst  = pcmsec;
            gctx->pick_msecfirst = pcmmsec;
        }

        if (gctx->edittime_callback) {
            /* Call callback */
            gctx->edittime_callback(
                gctx,
                gctx->pick_secfirst, gctx->pick_msecfirst,
                gctx->pick_seclast,  gctx->pick_mseclast); 
        }
        if (gctx->motionnotify_callback) {
            gctx->pcm_cursorvalue = pcmvalue;
            gctx->motionnotify_callback(widget, (GdkEventMotion *) event,
                                        gctx->motionnotify_callback_data);
      }
    }
    return FALSE;
}


static gint cb_pcmview_scroll_idle(gpointer data)
{
    pcmview_scroll_idlectx_t *idlectx = (pcmview_scroll_idlectx_t *) data;
    mpgedit_pcmview_ctx      *gctx    = idlectx->pcmviewctx;
    long                     startp;

    if (gctx->button_state == BUTTON_UP) {
        return FALSE;
    }
    if (gctx->scroll_stop) {
        return FALSE;
    }

    pcmdata_set_start(gctx, idlectx->sec_incr);
    gctx->scroll_sec += idlectx->sec_incr;
    /*
     * Compute the x position for the initial select pick.  Highlight
     * the waveform from the pick to the edge of the window.
     */
    startp = _mpgedit_pcmview_sec_to_xpos(gctx,
                                          gctx->pick_secfirst,
                                          gctx->pick_msecfirst);

    if (gctx->redraw_p >= gctx->drawing_area->allocation.width) {
        if (startp == -1) {
            startp = 0;
        }
        gctx->redraw_b  = startp;
        gctx->redraw_e  = gctx->drawing_area->allocation.width;
        gctx->redraw_gc = REDRAW_GC(gctx->drawing_area);
        draw_pcmdata_values_segment(gctx);
    }
    else {
        if (gctx->pick_secfirst >= gctx->pcm_seclast) {
            startp = gctx->drawing_area->allocation.width;
        }
        else if (startp == -1) {
            startp = 0;
        }
        gctx->redraw_b  = 0;
        gctx->redraw_e  = startp;
        gctx->redraw_gc = REDRAW_GC(gctx->drawing_area);
        draw_pcmdata_values_segment(gctx);
    }
    return TRUE;
}



static void enter_event(GtkWidget *widget,
                        GdkEventCrossing *event,
                        gpointer data)
{
    mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);

    D_printf(("enter_event <-- called:  x/y=%d:%d\n",
             (int) event->x, (int) event->y));
    gctx->scroll_stop = TRUE;
}



static void leave_event(GtkWidget *widget,
                        GdkEventCrossing *event,
                        gpointer data)
{
    mpgedit_pcmview_ctx      *gctx = GLOBAL_CTX(data);
    pcmview_scroll_idlectx_t *scrollctx;

    if (!gctx->button_state == BUTTON_DOWN) {
        return;
    }

    D_printf(("leave_event --> called: x/y=%d:%d w/h=%d:%d\n", 
             (int) event->x, (int) event->y,
              gctx->drawing_area->allocation.width,
              gctx->drawing_area->allocation.height));
    scrollctx = (pcmview_scroll_idlectx_t *) 
                    calloc(1, sizeof(pcmview_scroll_idlectx_t));
    if (!scrollctx) {
        return;
    }
    scrollctx->pcmviewctx = gctx;
    /* 
     * (+/-50) Allow a little slop around the window leave detection.
     * Win32 produces a value that is not the window edge, but a value
     * near the window edge.
     */
    if (event->x >= gctx->drawing_area->allocation.width - 50) {
        scrollctx->sec_incr   =  10;
    }
    else if (event->x <= 0 + 50) {
        scrollctx->sec_incr   = -10;
    }
    else {
        return;
    }
    gctx->scroll_stop = FALSE;
    gtk_timeout_add(100, cb_pcmview_scroll_idle, scrollctx);
}


static gint motion_notify_event(GtkWidget *widget,
                                GdkEventMotion *event,
                                gpointer data)
{
  mpgedit_pcmview_ctx *gctx = GLOBAL_CTX(data);
  int x, y;
  GdkModifierType state;
  int          step;
  int          eventx;
  int          do_draw = TRUE;

  if (event->is_hint) {
      gdk_window_get_pointer (event->window, &x, &y, &state);
  }
  else {
      x = (int) event->x;
      y = (int) event->y;
      state = event->state;
  }
    
  if (state & GDK_BUTTON1_MASK && gctx && gctx->pixmap) {
      if (gctx->direction == -1) {
          /*
           * First movement initially to left or right of initial 
           * button press
           */
          gctx->direction = x > gctx->eventx;
      }

      eventx = _mpgedit_pcmview_sec_to_xpos(
                   gctx, 
                   gctx->pick_secfirst,
                   gctx->pick_msecfirst);

      D_printf(("motion_notify_event: eventx=%d pick_secfirst=%ld pick_msecfirst=%ld\n", eventx,
                   gctx->pick_secfirst + gctx->pcm_secoffset,
                   gctx->pick_msecfirst));
       

      if (eventx == -1) {
          eventx = 0;
      }
      if (gctx->direction && x < eventx) {
          gctx->direction = !gctx->direction;

          gctx->redraw_b  = eventx;
          gctx->redraw_gc = widget->style->black_gc;
          D_printf(("<<<<<< %d:%d\n", eventx, gctx->redraw_p));
          draw_pcmdata_values_segment(gctx);
          gctx->redraw_gc = REDRAW_GC(widget);
          gctx->redraw_e  = eventx;
      }
      else if (!gctx->direction && x > eventx) {
          gctx->direction = !gctx->direction;

          gctx->redraw_b  = gctx->redraw_p;
          gctx->redraw_e  = eventx;
          gctx->redraw_gc = widget->style->black_gc;
          D_printf((">>>>> %d:%d\n", eventx, gctx->redraw_p));
          draw_pcmdata_values_segment(gctx);
          gctx->redraw_gc = REDRAW_GC(widget);
          gctx->redraw_b  = eventx;
      }

      /* Is movement left or right of previous? */
      step = gctx->redraw_e < x;
      if (gctx->direction) {
          if (step > 0) {
          /* 
           * Start redraw at old event position.  First value is initialized by
           * button down event. End redraw is the current motion event position.
           */
              gctx->redraw_b  = eventx;
              gctx->redraw_e  = x;
              gctx->redraw_gc = REDRAW_GC(widget);
          }
          else if (gctx->pick_secfirst < gctx->pcm_seclast) {
              gctx->redraw_b  = x;
              gctx->redraw_gc = widget->style->black_gc;
          }
          else {
              do_draw = FALSE;
          }
      }
      else {
          if (step > 0) {
              gctx->redraw_b  = gctx->redraw_p;
              gctx->redraw_e  = x;
              gctx->redraw_gc = widget->style->black_gc;
          }
          else {
              /*
               * Start redraw at old event position.  First value is
               * initialized by button down event. End redraw is the current
               * motion event position.
               */
              gctx->redraw_b  = x;
              gctx->redraw_gc = REDRAW_GC(widget);
          }
      }

      D_printf(("motion_notify_event (x/y)=%d/%d redraw=(%d->%d):(%d/%d)\n", 
                x, y, gctx->direction, step, gctx->redraw_b, gctx->redraw_e));
      if (do_draw) {
          draw_pcmdata_values_segment(gctx);
      }
      if (step <= 0) {
          gctx->redraw_e = gctx->redraw_b;
      }
      gctx->redraw_p = x;
      if (gctx->motionnotify_callback) {
          gctx->motionnotify_callback(widget, event,
                                      gctx->motionnotify_callback_data);
      }
  }
  return TRUE;
}


void quit(void)
{
  exit (0);
}


static long pcmview_seek_xpos(mpgedit_pcmview_ctx *ctx, int xpos)
{
    long pcmfpos;

    pcmfpos = mpgedit_pcmlevel_index_get_offset(
                  ctx->pcmfp,
                  ctx->pcmindex,
                  ctx->pcm_secfirst + ctx->pcm_secoffset);
    if (pcmfpos == -1) {
        return pcmfpos;
    }
    pcmfpos += xpos * 6;
    mpgedit_pcmlevel_seek(ctx->pcmfp, pcmfpos);
    return pcmfpos;
}


static void _mpgedit_pcmview_seek_sec(mpgedit_pcmview_ctx *ctx, long sec)
{
    long fpos;

    if (!ctx) {
        return;
    }

    sec += ctx->pcm_secoffset;
    fpos = mpgedit_pcmlevel_index_get_offset(ctx->pcmfp, ctx->pcmindex, sec);
    if (fpos != -1) {
        mpgedit_pcmlevel_seek(ctx->pcmfp, fpos);
        draw_pcmdata_values(ctx);
    }
}


static int _mpgedit_pcmview_sec_to_xpos(
              struct _mpgedit_pcmview_ctx *ctx,
              long lsec,
              long lmsec)
{
    gdouble pcmsecfirst;
    gdouble pcmmsecfirst;
    gdouble pcmseclast;
    gdouble pcmmseclast;
    gdouble sec  = (gdouble) lsec;
    gdouble msec = (gdouble) lmsec;

    if (!ctx) {
        return -1;
    }

    pcmsecfirst  = (gdouble) ctx->pcm_secfirst;
    pcmmsecfirst = (gdouble) ctx->pcm_msecfirst;
    pcmseclast   = (gdouble) ctx->pcm_seclast   + ctx->pcm_secoffset;
    pcmmseclast  = (gdouble) ctx->pcm_mseclast  + ctx->pcm_msecoffset;

    if (sec < pcmsecfirst || (sec == pcmsecfirst && msec < pcmmsecfirst) ||
        sec > pcmseclast || (sec == pcmseclast && msec > pcmmseclast))
    {
        return -1;
    }

    sec = (sec + msec/1000.0) - (pcmsecfirst + pcmmsecfirst/1000.0);
    return (int) (ctx->pixel_per_sec * sec);
}


static int _mpgedit_pcmview_get_data_xpos(
              struct _mpgedit_pcmview_ctx *ctx,
              int  *pcmvalue,
              long *pcmsec,
              long *pcmmsec)
{
    long pcmfpos_save;
    long pcmfpos;
    int  tmp_pcmvalue;
    long tmp_pcmsec;
    long tmp_pcmmsec;
    int  sts = 0;
    
    if (!ctx) {
        return sts;
    }

    /* Return 0:00.000 when X position is a negative value */
    if (ctx->eventx < 0) {
        if (pcmsec) {
            *pcmsec  = 0;
        }
        if (pcmmsec) {
            *pcmmsec  = 0;
        }
        return sts;
    }

    pcmfpos_save = mpgedit_pcmlevel_tell(ctx->pcmfp);
    pcmfpos = mpgedit_pcmlevel_index_get_offset(ctx->pcmfp,
                                                ctx->pcmindex,
                                                ctx->pcm_secfirst);
    if (pcmfpos == -1) {
        return sts;
    }
    pcmfpos += ctx->eventx * 6;
    sts = mpgedit_pcmlevel_seek(ctx->pcmfp, pcmfpos);
    if (sts == -1) {
        pcmfpos = mpgedit_pcmlevel_size(ctx->pcmfp);
        pcmfpos -= 6;
        mpgedit_pcmlevel_seek(ctx->pcmfp, pcmfpos);
    }
    sts = mpgedit_pcmlevel_read_entry(ctx->pcmfp,
                                      &tmp_pcmvalue, &tmp_pcmsec, &tmp_pcmmsec);
    if (sts) {
        if (pcmvalue) {
            *pcmvalue = tmp_pcmvalue;
        }
        if (tmp_pcmsec > ctx->pcm_seclast ||
            (tmp_pcmsec == ctx->pcm_seclast && tmp_pcmmsec > ctx->pcm_mseclast))
        {
            if (pcmsec) {
                *pcmsec  = ctx->pcm_seclast;
            }
            if (pcmmsec) {
                *pcmmsec = ctx->pcm_mseclast;
            }
        }
        else {
            if (pcmsec) {
                *pcmsec  = tmp_pcmsec;
            }
            if (pcmmsec) {
                *pcmmsec = tmp_pcmmsec;
            }
        }
    }
    mpgedit_pcmlevel_seek(ctx->pcmfp, pcmfpos_save);
    return sts;
}


static void _mpgedit_pcmview_draw_event(
                GtkWidget *widget, mpgedit_pcmview_ctx *ctx)
{
    draw_button_press(widget, (gpointer) ctx);
}


static void _mpgedit_pcmview_next_values(
                GtkWidget *widget, mpgedit_pcmview_ctx *ctx)
{
    next_button_press(widget, (gpointer) ctx);
}


static void _mpgedit_pcmview_previous_values(
                GtkWidget *widget, mpgedit_pcmview_ctx *ctx)
{
    prev_button_press(widget, (gpointer) ctx);
}


static void _mpgedit_pcmview_set_click_callback_pre(
                struct _mpgedit_pcmview_ctx *ctx,
                void   (*callback)(GtkWidget *widget, gpointer data))
{
    ctx->click_callback_pre = callback;
}


static void _mpgedit_pcmview_set_click_callback(
                struct _mpgedit_pcmview_ctx *ctx,
                void (*callback)(GtkWidget *, gpointer, int, gdouble, gdouble))
{
    ctx->click_callback = callback;
}


static void _mpgedit_pcmview_set_click_callback_post(
                struct _mpgedit_pcmview_ctx *ctx,
                void   (*callback)(GtkWidget *widget, gpointer data))
{
    ctx->click_callback_post = callback;
}


static void _mpgedit_pcmview_set_edittime_callback(
                struct _mpgedit_pcmview_ctx *ctx,
                void (*callback)(gpointer data,
                                 long secbegin, long msecbegin,
                                 long secend,   long msecend))
{
    ctx->edittime_callback = callback;
}


static void _mpgedit_pcmview_set_motionnotify_callback(
                struct _mpgedit_pcmview_ctx *ctx,
                void (*callback)(GtkWidget *widget,
                                 GdkEventMotion *event,
                                 gpointer data),
                gpointer data)
{
    ctx->motionnotify_callback      = callback;
    ctx->motionnotify_callback_data = data;
}


static void _mpgedit_pcmview_set_silence_db(
                struct _mpgedit_pcmview_ctx *ctx, int dblevel)
{
    ctx->decibels = dblevel;
}


static void _mpgedit_pcmview_set_filename(
                struct _mpgedit_pcmview_ctx *ctx, char *file)
{
    ctx->pcmfile = file;
}


static void _mpgedit_pcmview_set_click_button_mask(
                struct _mpgedit_pcmview_ctx *ctx, unsigned int mask)
{
    ctx->button_mask = mask;
}


static void _mpgedit_pcmview_set_drawn_flag(
                struct _mpgedit_pcmview_ctx *ctx, unsigned long value)
{
    ctx->drawn = value;
}


static void _mpgedit_pcmview_set_data(
                struct _mpgedit_pcmview_ctx *ctx, void *data)
{
    ctx->data = data;
}


static void *_mpgedit_pcmview_get_data(struct _mpgedit_pcmview_ctx *ctx)
{
    return ctx->data;
}


static unsigned long _mpgedit_pcmview_get_drawn_flag(
                struct _mpgedit_pcmview_ctx *ctx)
{
    return ctx->drawn;
}


static void _mpgedit_pcmview_close(struct _mpgedit_pcmview_ctx *ctx)
{
    close_viewer(ctx);
}


static void _mpgedit_pcmview_draw_cursor(
                struct _mpgedit_pcmview_ctx *ctx,
                int x,
                int percent)
{
    draw_cursor_line(ctx->drawing_area,
                     ctx,
                     percent,
                     x);
}


static void _mpgedit_pcmview_set_offset(
                struct _mpgedit_pcmview_ctx *ctx,
                unsigned long sec,
                unsigned long msec)
{
    ctx->pcm_secoffset  = sec;
    ctx->pcm_msecoffset = msec;
}


mpgedit_pcmview_ctx *mpgedit_pcmview_new(void)
{
  GtkWidget    *drawing_area;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *hbox_below;
  GtkWidget    *button_vbox;
  mpgedit_pcmview_ctx *globctx;

  globctx = (mpgedit_pcmview_ctx *) 
      calloc(1, sizeof(mpgedit_pcmview_ctx));
  if (!globctx) {
    return NULL;
  }

  globctx->seek_sec           = _mpgedit_pcmview_seek_sec;
  globctx->get_data_xpos      = _mpgedit_pcmview_get_data_xpos;
  globctx->sec_to_xpos        = _mpgedit_pcmview_sec_to_xpos;
  globctx->draw_event         = _mpgedit_pcmview_draw_event;
  globctx->draw_cursor        = _mpgedit_pcmview_draw_cursor;
  globctx->close              = _mpgedit_pcmview_close;
  globctx->next_values        = _mpgedit_pcmview_next_values;
  globctx->previous_values    = _mpgedit_pcmview_previous_values;
  globctx->set_click_callback_pre = 
                                _mpgedit_pcmview_set_click_callback_pre;
  globctx->set_click_callback = 
                                _mpgedit_pcmview_set_click_callback;
  globctx->set_click_callback_post = 
                                _mpgedit_pcmview_set_click_callback_post;
  globctx->set_edittime_callback = 
                                _mpgedit_pcmview_set_edittime_callback;
  globctx->set_motionnotify_callback = 
                                _mpgedit_pcmview_set_motionnotify_callback;
  globctx->set_silence_db     = _mpgedit_pcmview_set_silence_db;
  globctx->set_filename       = _mpgedit_pcmview_set_filename;
  globctx->set_button_mask    = _mpgedit_pcmview_set_click_button_mask;
  globctx->set_drawn_flag     = _mpgedit_pcmview_set_drawn_flag;
  globctx->set_data           = _mpgedit_pcmview_set_data;
  globctx->get_data           = _mpgedit_pcmview_get_data;
  globctx->get_drawn_flag     = _mpgedit_pcmview_get_drawn_flag;
  globctx->set_offset         = _mpgedit_pcmview_set_offset;

  vbox = gtk_vbox_new (FALSE, 0);
  globctx->pcmview = vbox;

  button_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (button_vbox);
  gtk_box_pack_start (GTK_BOX (vbox), button_vbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox);
  gtk_box_pack_start (GTK_BOX (button_vbox), hbox, FALSE, TRUE, 0);


  /* Create the drawing area */

  drawing_area = gtk_drawing_area_new ();
  gtk_box_pack_start (GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);

  hbox_below = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox_below);
  gtk_box_pack_start(GTK_BOX(vbox), hbox_below, FALSE, TRUE, 0);
  globctx->bottombox = hbox_below;

  gtk_widget_show (drawing_area);

  /* Signals used to handle backing pixmap */

  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
		    G_CALLBACK (expose_event), globctx);
  g_signal_connect (G_OBJECT (drawing_area),"configure_event",
		    G_CALLBACK (configure_event), globctx);

  /* Event signals */

  g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",
		    G_CALLBACK (motion_notify_event), globctx);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
		    G_CALLBACK (button_press_event_pre), globctx);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
		    G_CALLBACK (button_press_event), globctx);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
		    G_CALLBACK (button_press_event_post), globctx);
  g_signal_connect(G_OBJECT(drawing_area), "button_release_event",
                   G_CALLBACK(button_release_event), globctx);
  g_signal_connect(G_OBJECT(drawing_area), "enter_notify_event",
                   G_CALLBACK(enter_event), globctx);
  g_signal_connect(G_OBJECT(drawing_area), "leave_notify_event",
                   G_CALLBACK(leave_event), globctx);

  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_BUTTON_RELEASE_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK
			 | GDK_ENTER_NOTIFY_MASK
			 | GDK_LEAVE_NOTIFY_MASK
                        );

  globctx->topbox       = hbox;
  globctx->drawing_area = drawing_area;
  globctx->font_fixed   = gdk_font_load(
      "-misc-fixed-medium-r-*-*-*-140-*-*-*-*-*-*");

  return globctx;
}


#ifdef _UNIT_TEST
static void pcmview_click_callback(GtkWidget *widget, gpointer data,
                                   int button, gdouble x, gdouble y)
{
  mpgedit_pcmview_ctx *gctx = data;
   
  long pcmsec;
  long pcmmsec;

  gctx->get_data_xpos(gctx, NULL, &pcmsec, &pcmmsec);
  printf("%ld:%02ld.%03ld\n", pcmsec/60,
                              pcmsec%60,
                              pcmmsec);
}


void _test_seek_viewer(GtkWidget *widget, gpointer data)
{
  mpgedit_pcmview_ctx *gctx = data;

  gctx->seek_sec(gctx, 2000);
}


int main(int argc, char *argv[])
{
  char *file;
  mpgedit_pcmview_ctx *gctx;
  GtkWidget *window;
  GtkWidget *next_button;
  GtkWidget *quit_button;
  GtkWidget *prev_button;
  GtkWidget *seek_button;
  GtkWidget *draw_button;

  gtk_init (&argc, &argv);
  if (argc > 1) {
    file = argv[1];
  }
  else {
    file = "volumes.lvl";
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (window, "Test Input");

  gctx = mpgedit_pcmview_new();
  gtk_container_add(GTK_CONTAINER(window), gctx->pcmview);
  gtk_widget_show(gctx->pcmview);

  gctx->set_button_mask(gctx, 0x1);
  gctx->set_click_callback(gctx, pcmview_click_callback);
  gctx->set_silence_db(gctx, 4);
  gctx->set_filename(gctx, file);

  /* .. And a quit button */
  quit_button = gtk_button_new_with_label ("Quit");
  gtk_box_pack_start (GTK_BOX (gctx->topbox), quit_button, FALSE, FALSE, 0);
  gtk_widget_show (quit_button);


  /* add a next button */
  next_button = gtk_button_new_with_label("next");
  gtk_box_pack_start(GTK_BOX(gctx->topbox), next_button, FALSE, FALSE, 0);
  gtk_widget_show(next_button);
  g_signal_connect(G_OBJECT(next_button), "clicked",
                   G_CALLBACK(gctx->next_values), gctx);


  /* .. And add a previous button */
  prev_button = gtk_button_new_with_label("previous");
  gtk_box_pack_start(GTK_BOX(gctx->topbox), prev_button, FALSE, FALSE, 0);
  gtk_widget_show(prev_button);
  g_signal_connect(G_OBJECT(prev_button), "clicked",
                   G_CALLBACK(gctx->previous_values), gctx);


  seek_button = gtk_button_new_with_label("seek");
  gtk_box_pack_start(GTK_BOX(gctx->topbox), seek_button, FALSE, FALSE, 0);
  gtk_widget_show(seek_button);
  g_signal_connect(G_OBJECT(seek_button), "clicked",
                   G_CALLBACK(_test_seek_viewer), gctx);

  /* .. And a draw button */
  draw_button = gtk_button_new_with_label ("draw");
  gtk_box_pack_start (GTK_BOX (gctx->topbox), draw_button, FALSE, FALSE, 0);
  gtk_widget_show(draw_button);
  g_signal_connect(G_OBJECT(draw_button), "clicked",
                   G_CALLBACK(gctx->draw_event), gctx);

  g_signal_connect_swapped (G_OBJECT (quit_button), "clicked",
			    G_CALLBACK (gtk_widget_destroy),
			    window);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(quit), NULL);

  gtk_widget_show(window);
  gtk_main();
  return 0;
}
#endif
