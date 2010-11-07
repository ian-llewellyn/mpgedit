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


#ifndef _PCMVIEW_H
#define _PCMVIEW_H

#include <pcmlevel.h>

#define BUTTON_DOWN 1
#define BUTTON_UP   2
#define PCMVIEW_PICK_THRESHOLD 3

struct _mpgedit_pcmview_ctx
{
    void              (*click_callback)(
                          GtkWidget *widget,
                          gpointer data,
                          int     button_click,
                          gdouble x,
                          gdouble y);

    void              (*click_callback_pre)(
                          GtkWidget *widget,
                          gpointer data);
                    
    void              (*click_callback_post)(
                          GtkWidget *widget,
                          gpointer data);

    void              (*edittime_callback)(
                          gpointer data,
                          long secbegin, long msecbegin,
                          long secend,   long msecend);

    void              (*motionnotify_callback)(
                          GtkWidget      *widget,
                          GdkEventMotion *event,
                          gpointer       data);
    gpointer          motionnotify_callback_data;
                        
    void              (*seek_sec)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          long pcmseeksec);

    void              (*close)(
                          struct _mpgedit_pcmview_ctx *ctx);

    void              (*draw_cursor)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          int x,
                          int percent);

    void              (*draw_event)(
                          GtkWidget *widget,
                          struct _mpgedit_pcmview_ctx *ctx);

    void              (*next_values)(
                          GtkWidget *widget,
                          struct _mpgedit_pcmview_ctx *ctx);

    void              (*previous_values)(
                          GtkWidget *widget,
                          struct _mpgedit_pcmview_ctx *ctx);

    int               (*sec_to_xpos)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          long sec,
                          long msec);

    int               (*get_data_xpos)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          int  *pcmvalue, 
                          long *pcmsec, 
                          long *pcmmsec);

    void              (*set_click_callback_pre)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          void (*callback)(GtkWidget *widget, gpointer data));
                      
    void              (*set_click_callback_post)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          void (*callback)(GtkWidget *widget, gpointer data));
                      
    void              (*set_click_callback)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          void (*callback)(GtkWidget *, gpointer, int,
                                           gdouble, gdouble));

    void              (*set_edittime_callback)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          void (*callback)(gpointer data,
                                           long secbegin, long msecbegin,
                                           long secend,   long msecend));

    void              (*set_motionnotify_callback)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          void (*callback)(GtkWidget *widget,
                                           GdkEventMotion *event,
                                           gpointer data),
                          gpointer data);

    void              (*set_silence_db)(
                          struct _mpgedit_pcmview_ctx *ctx, int dblevel);

    void              (*set_filename)(
                          struct _mpgedit_pcmview_ctx *ctx, char *name);

    void              (*set_button_mask)(
                          struct _mpgedit_pcmview_ctx *ctx, unsigned int mask);

    void              (*set_drawn_flag)(
                          struct _mpgedit_pcmview_ctx *ctx, unsigned long val);

    void              (*set_data)(
                          struct _mpgedit_pcmview_ctx *ctx, void *data);

    void *            (*get_data)(struct _mpgedit_pcmview_ctx *ctx);

    unsigned long     (*get_drawn_flag)(
                          struct _mpgedit_pcmview_ctx *ctx);

    void              (*set_offset)(
                          struct _mpgedit_pcmview_ctx *ctx,
                          unsigned long sec, unsigned long msec);

#if 1
    int               decibels;
    GdkPixmap         *pixmap;
    GdkPixmap         *pixmap_shadow;
    GtkWidget         *pcmview;
    GtkWidget         *drawing_area;
    GtkWidget         *quit_button;
    GtkWidget         *topbox;
    GtkWidget         *bottombox;
    gpointer          data;
    char              *pcmfile;
    mpgedit_pcmfile_t        *pcmfp;
    mpgedit_pcmlevel_index_t *pcmindex;
    int               prev_len;
    int               prev_indx;
    int               pcm_avg;
    int               pcm_max;
    int               pcm_min;
    int               pcm_cursorvalue;

    long              pcm_secoffset;
    long              pcm_msecoffset;

    long              pcm_secfirst;
    long              pcm_msecfirst;
    long              pcm_seclast;
    long              pcm_mseclast;
    gdouble           pixel_per_sec;
    int               draw_button_ignore;
    int               eventx;
    int               eventy;
    GdkFont           *font_fixed;
    int               button_click;
    int               button_state;
    unsigned int      button_mask;
    long              scroll_sec;   /* Seconds view shifted since first pick */
    int               scroll_stop;
    int               drawn;        /* pcmview widget is rendered/refreshed */

#if 1 /* Perhaps create type for these values */
    /* Redraw PCM values from x->y using GC value */
    long              pick_xfirst;
    long              pick_yfirst;
    long              pick_secfirst;
    long              pick_msecfirst;
    long              pick_seclast;
    long              pick_mseclast;
    int               redraw_b;  /* begin    */
    int               redraw_e;  /* end      */
    int               redraw_p;  /* end      */
    int               direction; /* initially moved left or right of press */
    GdkGC             *redraw_gc;
#endif
#endif
};

typedef struct _mpgedit_pcmview_ctx mpgedit_pcmview_ctx;

mpgedit_pcmview_ctx *mpgedit_pcmview_new(void);


#endif
