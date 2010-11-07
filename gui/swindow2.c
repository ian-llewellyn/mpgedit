#include <stdio.h>
#include <gtk/gtk.h>

void destroy( GtkWidget *widget,
              gpointer   data )
{
    gtk_main_quit();
}


void cb_row_clicked(GtkWidget *widget, GdkEvent *event, gpointer *data)
{
    int value =(int) data;
#if 0
    gtk_window_activate_focus(GTK_WINDOW(widget));
#endif
    printf("Row %d selected\n", value);
}

GtkWidget *___make_row(char *file, int indx, int m, int s, int ms)
{
    GtkWidget *row;
    GtkWidget *label;
    GtkWidget *eb;
    char buf[128];

    eb = gtk_event_box_new();
    gtk_widget_show(eb);

    /* And bind an action to it */
    gtk_widget_set_events(eb, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(eb), "button_press_event",
                     G_CALLBACK(cb_row_clicked),(gpointer)indx);

    row = gtk_hbox_new(FALSE, 5);
    gtk_widget_show(row);

    gtk_container_add(GTK_CONTAINER(eb), row);

    label = gtk_label_new(file);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
    
    sprintf(buf, "%d", m);
    label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);

    sprintf(buf, "%d", s);
    label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);

    sprintf(buf, "%d", ms);
    label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
    return eb;
}

GtkWidget *___make_scrolled_window(void)
{
    GtkWidget *window;
    GtkWidget *scrolled_window;
    GtkWidget *vbox;
    GtkWidget *button;
    GtkWidget *row;
    char buffer[32];
    int i, j;

    /*
     * Create a new dialog window for the scrolled window to be packed into.
     *
     */
    window = gtk_dialog_new();
    g_signal_connect(G_OBJECT(window), "destroy",
                      G_CALLBACK(destroy), NULL);
    gtk_window_set_title(GTK_WINDOW(window), "GtkScrolledWindow example");
    gtk_container_set_border_width(GTK_CONTAINER(window), 0);
    gtk_widget_set_size_request(window, 300, 300);
    
    /* create a new scrolled window. */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    
    gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 10);
    
    /* the policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
     * GTK_POLICY_AUTOMATIC will automatically decide whether you need
     * scrollbars, whereas GTK_POLICY_ALWAYS will always leave the scrollbars
     * there.  The first one is the horizontal scrollbar, the second, 
     * the vertical. */
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    /* The dialog window is created with a vbox packed into it. */                                                              
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), scrolled_window, 
                        TRUE, TRUE, 0);
    gtk_widget_show(scrolled_window);
    
    vbox = gtk_vbox_new(FALSE, 5);
    
    gtk_scrolled_window_add_with_viewport(
                   GTK_SCROLLED_WINDOW(scrolled_window), vbox);
    gtk_widget_show(vbox);
    
    /* this simply creates a grid of toggle buttons on the table
     * to demonstrate the scrolled window. */
    for(i = 0; i < 20; i++) {
        row = ___make_row("file name", i, i, i*2, i*3-5);
        gtk_box_pack_start(GTK_BOX(vbox), row, FALSE, FALSE, 0);
    }
    return window;
}



GtkWidget *___make_dialog_informational(GtkWidget *widget,
                                        gint cancel_button,
                                        char *file_name,
                                        char *msg)
{
    GtkWidget *dialog;
    GtkWidget *label;


    if (cancel_button) {
        dialog = gtk_dialog_new_with_buttons(
                     msg,
                     GTK_WINDOW(widget),
                     GTK_DIALOG_MODAL,
                     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,
                     NULL);
    }
    else {
        dialog = gtk_dialog_new_with_buttons(
                     msg,
                     GTK_WINDOW(widget),
                     GTK_DIALOG_MODAL,
                     GTK_STOCK_OK,     GTK_RESPONSE_OK,
                     NULL);
    }

    label = gtk_label_new(msg);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       label, FALSE, FALSE, 0);
    if (file_name) {
        label = gtk_label_new(file_name);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                           label, FALSE, FALSE, 0);
    }
    return dialog;
}


void cb_dialog(GtkWidget *widget, gpointer *data)
{
    GtkWidget *window = (GtkWidget *) data;
    GtkWidget *dialog;

    printf("cb_button: called\n");

    dialog = ___make_dialog_informational(window, 1, NULL, "My Dialog");
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

}




int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *scrolled_window;
    GtkWidget *vbox;
    GtkWidget *button;
    GtkWidget *diag_button;
    GtkWidget *row;
    char buffer[32];
    int i, j;
    
    gtk_init(&argc, &argv);
    
    window = ___make_scrolled_window();
        
    /* Add a "close" button to the bottom of the dialog */
    button = gtk_button_new_with_label("close");
    g_signal_connect_swapped(G_OBJECT(button), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             window);

    /* Add a "dialog" button to the bottom of the dialog */
    diag_button = gtk_button_new_with_label("dialog");
    g_signal_connect(G_OBJECT(button), "clicked",
                             G_CALLBACK(cb_dialog),
                             window);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area),
                        diag_button, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(diag_button), "clicked", 
                     G_CALLBACK(cb_dialog), window);
    
    /* this makes it so the button is the default. */
    
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area),
                        button, TRUE, TRUE, 0);

    /* This grabs this button to be the default button. Simply hitting
     * the "Enter" key will cause this button to activate. */
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
    gtk_widget_show(diag_button);
    
    gtk_widget_show(window);
    
    gtk_main();
    
    return 0;
}


