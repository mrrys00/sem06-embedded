#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

static int counter = 0;

static void destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

void greet(GtkWidget *widget, gpointer data)
{
    // printf equivalent in GTK+
    g_print("Welcome to GTK\n");
    g_print("%s clicked %d times\n",
            (char *)data, ++counter);
}

static void initialize_window(GtkWidget *window)
{
    gtk_window_set_title(GTK_WINDOW(window), "My Window");          // Set window title
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);      // Set default size for the window
    g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL); // End application when close button clicked
}

int main(int argc, char *argv[])
{
    GtkWidget *window, *table, *label, *entry;
    GtkWidget *button;
    button = gtk_button_new_with_label("Click Me!");

    gtk_init(&argc, &argv);

    // Create the main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    initialize_window(window);

    /* Create a 1x2 table */
    table = gtk_table_new(1, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(window), table);

    /* create a new label. */
    label = gtk_label_new("Enter some text:");
    // gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
    gtk_table_set_homogeneous(GTK_TABLE(table), TRUE);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);

    // create a text box
    entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), entry, 0, 1, 0, 1);
    
    g_signal_connect(ATK_OBJECT(button), "clicked", G_CALLBACK(greet), "button");
    gtk_container_add(GTK_CONTAINER(window), button);

    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}