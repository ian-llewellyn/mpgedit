#include <gtk/gtk.h>
#include "playctrl.h"
#include <stdio.h>

int main(int argc, char *argv[]) 
{
    GtkWidget *window;
    GtkWidget *pixmap;
    GtkWidget *button;
    GtkRequisition pixmap_size;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    pixmap = xpm_pixmap_box(NULL, "close.xpm");
    gtk_widget_show(pixmap);
    gtk_widget_size_request(pixmap, &pixmap_size);
    printf("%d:%d\n", pixmap_size.width, pixmap_size.height);
#if 0
    gtk_window_resize(GTK_WINDOW(window), pixmap_size.width, pixmap_size.height);
#endif

    button = gtk_button_new();
    gtk_container_set_border_width(GTK_CONTAINER(button), 5);
    gtk_widget_show(button);
    gtk_container_add(GTK_CONTAINER(button), pixmap);
    
    gtk_container_add(GTK_CONTAINER(window), button);
    gtk_widget_show(window);
    gtk_main();
    return 1;
}
