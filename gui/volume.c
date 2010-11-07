/*
 * xmpgedit volume control
 *
 * Copyright (C) 2002-2006 Adam Bernstein
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

#include "../portability.h"
#include <gtk/gtk.h>
#include "volume.h"
#include "../playif.h"
#include "playctrl.h"

static void cb_volume_control(GtkWidget *widget, gpointer data)
{
    gdouble val;

    volume_control_ctx *ctx = (volume_control_ctx *) data;
    
    val = gtk_range_get_value(GTK_RANGE(widget));
    ctx->lvol = (int) val;
    ctx->rvol = (int) val;
    if (ctx->mixer_fd) {
        mpgedit_play_volume_set(ctx->mixer_fd, (int) val, (int) val);
    }
}


volume_control_ctx *___make_volume_control(void)
{
    GtkWidget     *box;
    GtkWidget     *scale;
    GtkObject     *adj;
    volume_control_ctx *vol;
    GtkWidget     *image;

    vol = (volume_control_ctx *) calloc(1, sizeof(volume_control_ctx));
    if (!vol) {
        return NULL;
    }
 
    box = gtk_vbox_new(FALSE, 0);

    image = xpm_pixmap_box(gtk_settings_get_default(), "volume1.xpm");
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

    adj = gtk_adjustment_new (0.0, 0.0, 110.0, 1.0, 10.0, 10.0);
    scale = gtk_vscale_new (GTK_ADJUSTMENT (adj));
    gtk_scale_set_digits(GTK_SCALE(scale), 0);
    gtk_range_set_inverted(GTK_RANGE(scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);

    gtk_box_pack_start(GTK_BOX(box), scale, TRUE, TRUE, 0);

    vol->box   = box;
    vol->scale = scale;
    vol->lvol  = vol->rvol = VOLUME_DEFAULT_VALUE;
    gtk_range_set_value(GTK_RANGE(scale), vol->lvol);
    g_signal_connect(G_OBJECT(scale), "value-changed",
                     G_CALLBACK(cb_volume_control), (gpointer) vol);
    return vol;
}

void volume_control_init_mixctx(volume_control_ctx *vol, void *ctx)
{
    vol->mixer_fd = ctx;
}


void volume_control_get_values(volume_control_ctx *vol, int *lvol, int *rvol)
{
    if (!vol) {
        return;
    }
    if (lvol) {
        *lvol = vol->lvol;
    }
    if (rvol) {
        *rvol = vol->rvol;
    }
}

#ifdef _UNIT_TEST
int main(int argc, char *argv[])
{
    GtkWidget *window;
    volume_control_ctx *volume;
    GtkRequisition scale_size;
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    volume = ___make_volume_control();
    gtk_container_add(GTK_CONTAINER(window), volume->box);
    gtk_widget_show_all(volume->box);
    gtk_widget_show(window);
    gtk_widget_size_request(volume->scale, &scale_size);
    gtk_widget_set_size_request(volume->scale, -1, scale_size.height * 3);
    gtk_main();
    return 0;
}
#endif
