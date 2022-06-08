#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <libgen.h>

typedef struct
{
   GtkWidget *menu_label;
   GtkWidget *menu;
   GtkWidget *new;
   GtkWidget *open;
   GtkWidget *save;
   GtkWidget *close;
   GtkWidget *separator;
   GtkWidget *quit;
} FileMenu;

typedef struct
{
   GtkWidget *menu_label;
   GtkWidget *menu;
   GtkWidget *cut;
   GtkWidget *copy;
   GtkWidget *paste;
} EditMenu;

typedef struct
{
   GtkWidget *scrolled_window;
   GtkWidget *textview;
   GtkWidget *tab_label;
} FileObject;

typedef struct
{
   GtkWidget *toplevel;
   GtkWidget *vbox;
   GtkWidget *notebook;
   GtkWidget *menubar;
   GtkWidget *toolbar;
   FileMenu *filemenu;
   EditMenu *editmenu;
} GreatGUIUI;

typedef struct
{
   gchar *filename;
   gint tab_number;
} FileData;


static GList *filename_data = NULL;    // opened files list

static void quit_application(GtkWidget *, gpointer);
static void text_edit_init_GUI(GreatGUIUI *);
static void text_edit_create_menus(GreatGUIUI *);
static void text_edit_create_toolbar_items(GreatGUIUI *);
FileObject *text_edit_file_new(void);
static void text_edit_tab_new_with_file(GtkMenuItem *, GtkNotebook *);
static void text_edit_open_file(GtkMenuItem *, GtkNotebook *);
static void text_edit_cut_to_clipboard(GtkMenuItem *, GtkNotebook *);
static void text_edit_copy_to_clipboard(GtkMenuItem *, GtkNotebook *);
static void text_edit_paste_from_clipboard(GtkMenuItem *, GtkNotebook *);
static void text_edit_close_file(GtkMenuItem *, GtkNotebook *);
static void text_edit_save_file(GtkMenuItem *, GtkNotebook *);
static void text_edit_register_filename(gchar *fname, gint tab_num);
static gchar *text_edit_get_filename(gint tab_num);

int main(int argc, char *argv[])
{
   GreatGUIUI app;
   gtk_init(&argc, &argv);
   app.toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL); // windpw

   gtk_window_set_title(GTK_WINDOW(app.toplevel), "GreatGUI");
   gtk_window_set_default_size(GTK_WINDOW(app.toplevel), 650, 350);

   g_signal_connect(G_OBJECT(app.toplevel), "destroy", G_CALLBACK(quit_application), NULL);
   text_edit_init_GUI(&app);        // init gui
   gtk_widget_show_all(app.toplevel);
   gtk_main();

   return 0;
}

static void quit_application(GtkWidget *window, gpointer data)
{
   gtk_main_quit();
}

static void text_edit_init_GUI(GreatGUIUI *app)
{
   FileObject *file = text_edit_file_new();
   text_edit_register_filename("New", 0);

   app->vbox = gtk_box_new(TRUE, 0);      // vertically position - true :D
   app->notebook = gtk_notebook_new();
   app->menubar = gtk_menu_bar_new();

   text_edit_create_menus(app);
   text_edit_create_toolbar_items(app);

   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(app->notebook), TRUE);
   gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), file->scrolled_window, file->tab_label);
   gtk_box_pack_start(GTK_BOX(app->vbox), app->notebook, TRUE, TRUE, 0);
   gtk_container_add(GTK_CONTAINER(app->toplevel), app->vbox);
}

static void text_edit_create_menus(GreatGUIUI *app)
{
   FileMenu *file;
   EditMenu *edit;
   GtkAccelGroup *group = gtk_accel_group_new();

   app->filemenu = g_new(FileMenu, 1);
   file = app->filemenu;

   app->editmenu = g_new(EditMenu, 1);
   edit = app->editmenu;

   gtk_window_add_accel_group(GTK_WINDOW(app->toplevel), group);

   file->menu_label = gtk_menu_item_new_with_label("File");
   file->menu = gtk_menu_new();
   file->new = gtk_menu_item_new_with_label("New");
   file->open = gtk_menu_item_new_with_label("Open");
   file->save = gtk_menu_item_new_with_label("Save");
   file->close = gtk_menu_item_new_with_label("Close");
   file->separator = gtk_separator_menu_item_new();
   file->quit = gtk_menu_item_new_with_label("Quit");
   gtk_menu_item_set_submenu(GTK_MENU_ITEM(file->menu_label), file->menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(file->menu), file->new);
   gtk_menu_shell_append(GTK_MENU_SHELL(file->menu), file->open);
   gtk_menu_shell_append(GTK_MENU_SHELL(file->menu), file->save);
   gtk_menu_shell_append(GTK_MENU_SHELL(file->menu), file->close);
   gtk_menu_shell_append(GTK_MENU_SHELL(file->menu), file->separator);
   gtk_menu_shell_append(GTK_MENU_SHELL(file->menu), file->quit);
   gtk_menu_set_accel_group(GTK_MENU(file->menu), group);

   edit->menu_label = gtk_menu_item_new_with_label("Edit");
   edit->menu = gtk_menu_new();
   edit->copy = gtk_menu_item_new_with_label("Copy");
   edit->cut = gtk_menu_item_new_with_label("Cut");
   edit->paste = gtk_menu_item_new_with_label("Paste");
   gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit->menu_label), edit->menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(edit->menu), edit->cut);
   gtk_menu_shell_append(GTK_MENU_SHELL(edit->menu), edit->copy);
   gtk_menu_shell_append(GTK_MENU_SHELL(edit->menu), edit->paste);
   gtk_menu_set_accel_group(GTK_MENU(edit->menu), group);

   gtk_menu_shell_append(GTK_MENU_SHELL(app->menubar), file->menu_label);
   gtk_menu_shell_append(GTK_MENU_SHELL(app->menubar), edit->menu_label);

   g_signal_connect(G_OBJECT(file->new), "activate", G_CALLBACK(text_edit_tab_new_with_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->open), "activate", G_CALLBACK(text_edit_open_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->save), "activate", G_CALLBACK(text_edit_save_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->close), "activate", G_CALLBACK(text_edit_close_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(edit->cut), "activate", G_CALLBACK(text_edit_cut_to_clipboard), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(edit->copy), "activate", G_CALLBACK(text_edit_copy_to_clipboard), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(edit->paste), "activate", G_CALLBACK(text_edit_paste_from_clipboard), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->quit), "activate", G_CALLBACK(quit_application), NULL);

   gtk_box_pack_start(GTK_BOX(app->vbox), app->menubar, FALSE, FALSE, 0);     // add menu
}

static void text_edit_create_toolbar_items(GreatGUIUI *app)
{
   GtkWidget *toolbar;
   GtkToolItem *new, *open, *save;

   toolbar = app->toolbar = gtk_toolbar_new();
   gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), TRUE);
   new = gtk_tool_button_new(gtk_image_new_from_icon_name("document-new", GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
   open = gtk_tool_button_new(gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
   save = gtk_tool_button_new(gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
   gtk_toolbar_insert(GTK_TOOLBAR(toolbar), new, 0);
   gtk_toolbar_insert(GTK_TOOLBAR(toolbar), open, 1);
   gtk_toolbar_insert(GTK_TOOLBAR(toolbar), save, 2);

   g_signal_connect_swapped(G_OBJECT(new), "clicked", G_CALLBACK(gtk_menu_item_activate), (gpointer)app->filemenu->new);
   g_signal_connect_swapped(G_OBJECT(open), "clicked", G_CALLBACK(gtk_menu_item_activate), (gpointer)app->filemenu->open);
   g_signal_connect_swapped(G_OBJECT(save), "clicked", G_CALLBACK(gtk_menu_item_activate), (gpointer)app->filemenu->save);
   gtk_box_pack_start(GTK_BOX(app->vbox), toolbar, FALSE, FALSE, 0);
}

FileObject *text_edit_file_new(void)
{
   FileObject *new_file = g_new(FileObject, 1);

   new_file->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
   new_file->textview = gtk_text_view_new();
   new_file->tab_label = gtk_label_new("Untitled");

   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(new_file->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(new_file->scrolled_window), GTK_SHADOW_IN);
   gtk_container_set_border_width(GTK_CONTAINER(new_file->scrolled_window), 3);
   gtk_text_view_set_left_margin(GTK_TEXT_VIEW(new_file->textview), 3);
   gtk_text_view_set_right_margin(GTK_TEXT_VIEW(new_file->textview), 3);
   gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(new_file->textview), 1);

   gtk_container_add(GTK_CONTAINER(new_file->scrolled_window), new_file->textview);

   return new_file;
}

static void text_edit_tab_new_with_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   FileObject *f = text_edit_file_new();
   gint current_tab;

   current_tab = gtk_notebook_append_page(notebook, f->scrolled_window, f->tab_label);
   text_edit_register_filename("Untitled", current_tab);
   gtk_widget_show_all(GTK_WIDGET(notebook));
}

static void text_edit_open_file(GtkMenuItem *menu_item,
                                GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *dialog;
   GtkWidget *save_dialog, *error_dialog;
   gint current_page;
   gint id;
   gint offset;
   gchar *filename;
   gchar *contents;
   GtkWidget *scrolled_win;
   GtkWidget *view;
   GtkTextBuffer *buffer;
   GtkWidget *tab_name;
   GtkTextIter start, end;
   GtkTextMark *mark;
   GtkWidget *prompt_label;
   GtkWidget *content_area;

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));

   view = (GTK_IS_TEXT_VIEW(child_list->data) ? child_list->data : NULL);

   if (view != NULL)
   {
      dialog = gtk_file_chooser_dialog_new("Open A File", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, "Open", GTK_RESPONSE_ACCEPT, "Cancel", GTK_RESPONSE_REJECT, NULL);
      id = gtk_dialog_run(GTK_DIALOG(dialog));
      tab_name = gtk_notebook_get_tab_label(notebook, scrolled_win);

      switch (id)
      {
      case GTK_RESPONSE_ACCEPT:
         filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

         buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
         gtk_text_buffer_get_end_iter(buffer, &end);
         offset = gtk_text_iter_get_offset(&end);
         if (offset > 0)
         {
            save_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, NULL);

            gtk_dialog_add_buttons(GTK_DIALOG(save_dialog), "Save", GTK_RESPONSE_ACCEPT, "Delete", GTK_RESPONSE_CLOSE, "Cancel", GTK_RESPONSE_CANCEL, NULL);

            prompt_label = gtk_label_new("Save buffer contents?");
            content_area = gtk_dialog_get_content_area(GTK_DIALOG(save_dialog));

            gtk_box_pack_start(GTK_BOX(content_area), prompt_label, FALSE, FALSE, 0);
            gtk_widget_show_all(save_dialog);
            gtk_widget_hide(dialog);

            id = gtk_dialog_run(GTK_DIALOG(save_dialog));

            switch (id)
            {
            case GTK_RESPONSE_ACCEPT:
               text_edit_save_file(NULL, notebook);
               text_edit_register_filename(filename, current_page);
               break;

            case GTK_RESPONSE_CLOSE:
               gtk_text_buffer_get_bounds(buffer, &start, &end);
               gtk_text_buffer_delete(buffer, &start, &end);
               break;

            case GTK_RESPONSE_CANCEL:
               gtk_widget_destroy(save_dialog);
               return;
            }
            gtk_widget_destroy(save_dialog);
         }

         if (g_file_test(filename, G_FILE_TEST_EXISTS))
         {
            g_file_get_contents(filename, &contents, NULL, NULL);
            mark = gtk_text_buffer_get_insert(buffer);
            gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
            gtk_text_buffer_set_text(buffer, contents, -1);
            text_edit_register_filename(filename, current_page);
            gtk_label_set_text(GTK_LABEL(tab_name), basename(filename));
         }
         else
         {
            error_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL);  // non existing file

            gtk_dialog_run(GTK_DIALOG(error_dialog));
            gtk_widget_destroy(error_dialog);
         }
         break;

      case GTK_RESPONSE_REJECT:
         break;
      }
      gtk_widget_destroy(dialog);
   }
}

static void text_edit_cut_to_clipboard(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *textview;
   GtkTextBuffer *buffer;
   GtkWidget *scrolled_win;
   gint current_page;
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   textview = child_list->data;

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
}

static void text_edit_copy_to_clipboard(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *textview;
   GtkTextBuffer *buffer;
   GtkWidget *scrolled_win;
   gint current_page;
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   textview = child_list->data;

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   gtk_text_buffer_copy_clipboard(buffer, clipboard);
}

static void text_edit_paste_from_clipboard(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *textview;
   GtkTextBuffer *buffer;
   GtkWidget *scrolled_win;
   gint current_page;
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   textview = child_list->data;

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
   gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
}

static void text_edit_close_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GtkWidget *scrolled_win;
   gint current_page;

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);

   gtk_widget_destroy(scrolled_win); /* Remove current tab */
}

static void text_edit_save_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *dialog;
   GtkWidget *textview;
   GtkTextBuffer *buffer;
   GtkWidget *scrolled_win;
   gint current_page;
   gint response;
   GtkWidget *tab_label;
   GtkTextIter start, end;
   gchar *filename;
   gchar *contents;

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   textview = child_list->data;
   tab_label = gtk_notebook_get_tab_label(notebook, scrolled_win);

   if (strcmp(gtk_label_get_text(GTK_LABEL(tab_label)), "Untitled") == 0)
   {

      dialog = gtk_file_chooser_dialog_new("Save File", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, "Save", GTK_RESPONSE_APPLY, "Cancel", GTK_RESPONSE_CANCEL, NULL);

      response = gtk_dialog_run(GTK_DIALOG(dialog));

      if (response == GTK_RESPONSE_APPLY)
      {
         filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

         buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
         gtk_text_buffer_get_bounds(buffer, &start, &end);
         contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

         g_file_set_contents(filename, contents, -1, NULL);

         text_edit_register_filename(filename, current_page); // ADDED

         gtk_label_set_text(GTK_LABEL(tab_label), basename(filename));
      }
      else if (response == GTK_RESPONSE_CANCEL)
      {
         gtk_widget_destroy(dialog);
         return;
      }

      gtk_widget_destroy(dialog);
   }
   else
   {
      // edit existing file
      filename = text_edit_get_filename(current_page);

      buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
      gtk_text_buffer_get_bounds(buffer, &start, &end);
      contents = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

      g_file_set_contents(filename, contents, -1, NULL);
   }
}

static void text_edit_register_filename(gchar *fname, gint tab_num)
{
   // manage file and its saves
   FileData *f = g_new(FileData, 1);
   GList *node = g_list_alloc();

   f->filename = fname;
   f->tab_number = tab_num;

   node->data = f;

   if (filename_data == NULL)
      filename_data = node;         // first file linked list node
   else
   {
      GList *list = filename_data;

      while (list != NULL)
      {
         if (((FileData *)list->data)->tab_number == tab_num)
         {
            ((FileData *)list->data)->filename = fname;
            break;
         }
         else
            list = g_list_next(list);
      }
   }
}

static gchar *text_edit_get_filename(gint tab_num)
{
   GList *list = filename_data;

   while (list != NULL)
   {
      if (((FileData *)list->data)->tab_number == tab_num)
         return ((FileData *)list->data)->filename;
      else
         list = g_list_next(filename_data);
   }
   return NULL;
}