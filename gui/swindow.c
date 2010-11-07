#include <stdio.h>
#include <gtk/gtk.h>

void destroy( GtkWidget *widget,
              gpointer   data )
{
    gtk_main_quit();
}


void cb_window_button_press(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GdkEventButton *event_button = (GdkEventButton *) event;

    printf("cb_window_button_press: called\n");
    if (event->type == GDK_BUTTON_PRESS) {
        printf("cb_window_button_press: event_button: %d\n",
               event_button->button);
    }
}


void ___add_row(GtkWidget *clist, char *file, int i, int m, int s, int ms)
{
    char *item[4];
    char buf0[128];
    char buf1[128];
    char buf2[128];

    buf0[0] = buf1[0] = buf2[0] = '\0';

    
    item[0] = file;
    sprintf(buf1, "%d:%d.%d", m, s, ms);
    item[1] = buf1;
    sprintf(buf2, "%d:%d.%d", m + s, s + ms, ms);
    item[2] = buf2;
    item[3] = NULL;
    gtk_clist_insert(GTK_CLIST(clist), i, item);
}


GtkWidget *___make_clist_window(GtkWidget **rlist)
{
    GtkWidget *scrolled_window;
    GtkWidget *list;
    char *list_titles[3];

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
    list = gtk_clist_new_with_titles(3, list_titles);
    gtk_widget_show(list);

    gtk_container_add(GTK_CONTAINER(scrolled_window), list);

    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 1, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(list), 2, TRUE);
    gtk_clist_column_titles_passive(GTK_CLIST(list));
    gtk_clist_set_column_justification(GTK_CLIST(list), 1, GTK_JUSTIFY_FILL);
    gtk_clist_set_column_justification(GTK_CLIST(list), 2, GTK_JUSTIFY_FILL);
    gtk_clist_set_selection_mode(GTK_CLIST(list), GTK_SELECTION_BROWSE);

    *rlist = list;
    return scrolled_window;
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


int modal_dialog_idleproc(gpointer data)
{
    if (gdk_pointer_is_grabbed()) {
        return TRUE;
    }

    printf("modal_dialog_idleproc: called\n");
    cb_dialog(NULL, data);
    return FALSE;
}


void cb_row_clicked(GtkWidget *widget,
                    int row,
                    int column,
                    GdkEventButton *event,
                    gpointer data)

{
    printf("cb_row_clicked: Row %d column %d selected; button=%d\n",
           row, column, event ? event->button : -1);

    if (!event || (event && event->button != 1)) return;

    gtk_idle_add(modal_dialog_idleproc, data);
}


int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *dialog_button;
    GtkWidget *box;
    GtkWidget *scrolled_window;
    GtkWidget *clist;
    GtkWidget *buttons_hbox;

    int i;
    
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    g_signal_connect(G_OBJECT(window), "destroy",
                      G_CALLBACK(destroy), NULL);
    gtk_window_set_title(GTK_WINDOW(window), "GtkScrolledWindow example");
    gtk_container_set_border_width(GTK_CONTAINER(window), 0);
    gtk_widget_set_size_request(window, 300, 300);

    g_signal_connect(GTK_CONTAINER(window), "button-press-event",
                     G_CALLBACK(cb_window_button_press), NULL);
    box = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(box);

    buttons_hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(buttons_hbox);

    gtk_container_add(GTK_CONTAINER(window), box);

    scrolled_window = ___make_clist_window(&clist);
    gtk_widget_show(scrolled_window);

    /* this simply creates a grid of toggle buttons on the table
     * to demonstrate the scrolled window.
     */
    for(i = 0; i < 200; i++) {
        ___add_row(clist, "file name", i, i, i*2, i*3-5);
    }

    gtk_box_pack_start(GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), buttons_hbox, FALSE, FALSE, 20);

    gtk_clist_set_button_actions(GTK_CLIST(clist), 0, GTK_BUTTON_SELECTS);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 1, GTK_BUTTON_SELECTS);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 2, GTK_BUTTON_SELECTS);
        
    /* Add a "close" button to the bottom of the dialog */
    button = gtk_button_new_with_label("close");
    g_signal_connect_swapped(G_OBJECT(button), "clicked",
                             G_CALLBACK(gtk_widget_destroy),
                             window);

    /* Add a "dialog" button to the bottom of the dialog */
    dialog_button = gtk_button_new_with_label("dialog");

    g_signal_connect(G_OBJECT(dialog_button), "clicked", 
                     G_CALLBACK(cb_dialog), window);

    g_signal_connect(GTK_CLIST(clist), "select-row",
                     G_CALLBACK(cb_row_clicked), window);
    
    gtk_box_pack_start(GTK_BOX(buttons_hbox),
                        dialog_button, TRUE, TRUE, 0);
    
    /* this makes it so the button is the default. */
    
    GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(buttons_hbox),
                        button, TRUE, TRUE, 0);

    /* This grabs this button to be the default button. Simply hitting
     * the "Enter" key will cause this button to activate. */
    gtk_widget_grab_default(button);
    gtk_widget_show(button);
    gtk_widget_show(dialog_button);
    
    gtk_widget_show(window);
    
    gtk_main();
    
    return 0;
}
