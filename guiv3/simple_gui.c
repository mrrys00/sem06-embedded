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
   GtkWidget *text_view;
   GtkWidget *tab_label;
} FileObject;

typedef struct
{
   GtkWidget *toplevel;
   GtkWidget *tool_menu_box;
   GtkWidget *notebook;
   GtkWidget *menu_bar;
   GtkWidget *tool_bar;
   FileMenu *file_menu;
   EditMenu *edit_menu;
} GreatGUI;

typedef struct
{
   gchar *filename;
   gint nutebook_num;
} FileData;

static GList *filename_data = NULL; // opened files list

static void notepad_init_GUI(GreatGUI *);
static void notepad_create_menus(GreatGUI *);
static void notepad_create_toolbar_items(GreatGUI *);
FileObject *notepad_file_new(void);
static void notepad_tab_new_with_file(GtkMenuItem *, GtkNotebook *);
static void notepad_open_file(GtkMenuItem *, GtkNotebook *);

static void notepad_cut_to_clipboard(GtkMenuItem *, GtkNotebook *);
static void notepad_copy_to_clipboard(GtkMenuItem *, GtkNotebook *);
static void notepad_paste_from_clipboard(GtkMenuItem *, GtkNotebook *);
static void notepad_close_file(GtkMenuItem *, GtkNotebook *);
static void notepad_save_file(GtkMenuItem *, GtkNotebook *);

static void notepad_register_filename(gchar *fname, gint tab_num);
static gchar *notepad_get_filename(gint tab_num);
static void quit_application(GtkWidget *, gpointer);

int main(int argc, char *argv[])
{
   GreatGUI app;
   gtk_init(&argc, &argv);
   app.toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);   // window

   gtk_window_set_title(GTK_WINDOW(app.toplevel), "Radio notepad");
   gtk_window_set_default_size(GTK_WINDOW(app.toplevel), 650, 350);

   g_signal_connect(G_OBJECT(app.toplevel), "destroy", G_CALLBACK(quit_application), NULL);
   notepad_init_GUI(&app);          // init gui
   gtk_widget_show_all(app.toplevel);
   gtk_main();

   return 0;
}

static void notepad_init_GUI(GreatGUI *app)
{
   FileObject *file = notepad_file_new();
   notepad_register_filename("New", 0);

   app->tool_menu_box = gtk_box_new(TRUE, 0); // vertically position - true :D
   app->notebook = gtk_notebook_new();
   app->menu_bar = gtk_menu_bar_new();

   notepad_create_menus(app);
   notepad_create_toolbar_items(app);

   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(app->notebook), TRUE);
   gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), file->scrolled_window, file->tab_label);
   gtk_box_pack_start(GTK_BOX(app->tool_menu_box), app->notebook, TRUE, TRUE, 0);
   gtk_container_add(GTK_CONTAINER(app->toplevel), app->tool_menu_box);
}

static void notepad_create_menus(GreatGUI *app)
{
   FileMenu *file;
   EditMenu *edit;
   GtkAccelGroup *group = gtk_accel_group_new();

   app->file_menu = g_new(FileMenu, 1);
   file = app->file_menu;

   app->edit_menu = g_new(EditMenu, 1);
   edit = app->edit_menu;

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

   gtk_menu_shell_append(GTK_MENU_SHELL(app->menu_bar), file->menu_label);
   gtk_menu_shell_append(GTK_MENU_SHELL(app->menu_bar), edit->menu_label);

   g_signal_connect(G_OBJECT(file->new), "activate", G_CALLBACK(notepad_tab_new_with_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->open), "activate", G_CALLBACK(notepad_open_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->save), "activate", G_CALLBACK(notepad_save_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->close), "activate", G_CALLBACK(notepad_close_file), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(edit->cut), "activate", G_CALLBACK(notepad_cut_to_clipboard), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(edit->copy), "activate", G_CALLBACK(notepad_copy_to_clipboard), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(edit->paste), "activate", G_CALLBACK(notepad_paste_from_clipboard), (gpointer)app->notebook);
   g_signal_connect(G_OBJECT(file->quit), "activate", G_CALLBACK(quit_application), NULL);

   gtk_box_pack_start(GTK_BOX(app->tool_menu_box), app->menu_bar, FALSE, FALSE, 0); // add menu
}

static void notepad_create_toolbar_items(GreatGUI *app)
{
   GtkWidget *tool_bar;
   GtkToolItem *new, *open, *save;

   tool_bar = app->tool_bar = gtk_toolbar_new();
   gtk_toolbar_set_show_arrow(GTK_TOOLBAR(tool_bar), TRUE);
   new = gtk_tool_button_new(gtk_image_new_from_icon_name("document-new", GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
   open = gtk_tool_button_new(gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
   save = gtk_tool_button_new(gtk_image_new_from_icon_name("document-save", GTK_ICON_SIZE_SMALL_TOOLBAR), NULL);
   gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), new, 0);
   gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), open, 1);
   gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), save, 2);

   g_signal_connect_swapped(G_OBJECT(new), "clicked", G_CALLBACK(gtk_menu_item_activate), (gpointer)app->file_menu->new);
   g_signal_connect_swapped(G_OBJECT(open), "clicked", G_CALLBACK(gtk_menu_item_activate), (gpointer)app->file_menu->open);
   g_signal_connect_swapped(G_OBJECT(save), "clicked", G_CALLBACK(gtk_menu_item_activate), (gpointer)app->file_menu->save);
   gtk_box_pack_start(GTK_BOX(app->tool_menu_box), tool_bar, FALSE, FALSE, 0);
}

FileObject *notepad_file_new(void)
{
   FileObject *new_file = g_new(FileObject, 1);

   new_file->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
   new_file->text_view = gtk_text_view_new();
   new_file->tab_label = gtk_label_new("New");

   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(new_file->scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(new_file->scrolled_window), GTK_SHADOW_IN);
   gtk_container_set_border_width(GTK_CONTAINER(new_file->scrolled_window), 3);
   gtk_text_view_set_left_margin(GTK_TEXT_VIEW(new_file->text_view), 3);
   gtk_text_view_set_right_margin(GTK_TEXT_VIEW(new_file->text_view), 3);
   gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(new_file->text_view), 1);

   gtk_container_add(GTK_CONTAINER(new_file->scrolled_window), new_file->text_view);

   return new_file;
}

static void notepad_tab_new_with_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   FileObject *f = notepad_file_new();
   gint current_tab;

   current_tab = gtk_notebook_append_page(notebook, f->scrolled_window, f->tab_label);
   notepad_register_filename("New", current_tab);
   gtk_widget_show_all(GTK_WIDGET(notebook));
}

static void notepad_open_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
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
   GtkTextBuffer *buf;
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

         buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
         gtk_text_buffer_get_end_iter(buf, &end);
         offset = gtk_text_iter_get_offset(&end);
         if (offset > 0)
         {
            save_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, NULL);

            gtk_dialog_add_buttons(GTK_DIALOG(save_dialog), "Save", GTK_RESPONSE_ACCEPT, "Delete", GTK_RESPONSE_CLOSE, "Cancel", GTK_RESPONSE_CANCEL, NULL);

            prompt_label = gtk_label_new("Save buf contents?");
            content_area = gtk_dialog_get_content_area(GTK_DIALOG(save_dialog));

            gtk_box_pack_start(GTK_BOX(content_area), prompt_label, FALSE, FALSE, 0);
            gtk_widget_show_all(save_dialog);
            gtk_widget_hide(dialog);

            id = gtk_dialog_run(GTK_DIALOG(save_dialog));

            switch (id)
            {
            case GTK_RESPONSE_ACCEPT:
               notepad_save_file(NULL, notebook);
               notepad_register_filename(filename, current_page);
               break;

            case GTK_RESPONSE_CLOSE:
               gtk_text_buffer_get_bounds(buf, &start, &end);
               gtk_text_buffer_delete(buf, &start, &end);
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
            mark = gtk_text_buffer_get_insert(buf);
            gtk_text_buffer_get_iter_at_mark(buf, &start, mark);
            gtk_text_buffer_set_text(buf, contents, -1);
            notepad_register_filename(filename, current_page);
            gtk_label_set_text(GTK_LABEL(tab_name), basename(filename));
         }
         else
         {
            error_dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL); // non existing file

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

static void notepad_cut_to_clipboard(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *text_view;
   GtkTextBuffer *buf;
   GtkWidget *scrolled_win;
   gint current_page;
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   text_view = child_list->data;

   buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   gtk_text_buffer_cut_clipboard(buf, clipboard, TRUE);
}

static void notepad_copy_to_clipboard(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *text_view;
   GtkTextBuffer *buf;
   GtkWidget *scrolled_win;
   gint current_page;
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   text_view = child_list->data;

   buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   gtk_text_buffer_copy_clipboard(buf, clipboard);
}

static void notepad_paste_from_clipboard(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *text_view;
   GtkTextBuffer *buf;
   GtkWidget *scrolled_win;
   gint current_page;
   GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);
   child_list = gtk_container_get_children(GTK_CONTAINER(scrolled_win));
   text_view = child_list->data;

   buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
   gtk_text_buffer_paste_clipboard(buf, clipboard, NULL, TRUE);
}

static void notepad_close_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GtkWidget *scrolled_win;
   gint current_page;

   current_page = gtk_notebook_get_current_page(notebook);
   scrolled_win = gtk_notebook_get_nth_page(notebook, current_page);

   gtk_widget_destroy(scrolled_win); /* Remove current tab */
}

static void notepad_save_file(GtkMenuItem *menu_item, GtkNotebook *notebook)
{
   GList *child_list;
   GtkWidget *dialog;
   GtkWidget *text_view;
   GtkTextBuffer *buf;
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
   text_view = child_list->data;
   tab_label = gtk_notebook_get_tab_label(notebook, scrolled_win);

   if (strcmp(gtk_label_get_text(GTK_LABEL(tab_label)), "New") == 0)
   {

      dialog = gtk_file_chooser_dialog_new("Save File", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, "Save", GTK_RESPONSE_APPLY, "Cancel", GTK_RESPONSE_CANCEL, NULL);

      response = gtk_dialog_run(GTK_DIALOG(dialog));

      if (response == GTK_RESPONSE_APPLY)
      {
         filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

         buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
         gtk_text_buffer_get_bounds(buf, &start, &end);
         contents = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

         g_file_set_contents(filename, contents, -1, NULL);

         notepad_register_filename(filename, current_page);

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
      filename = notepad_get_filename(current_page);

      buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
      gtk_text_buffer_get_bounds(buf, &start, &end);
      contents = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

      g_file_set_contents(filename, contents, -1, NULL);
   }
}

static void notepad_register_filename(gchar *fname, gint tab_num)
{
   // manage file and its saves
   FileData *f = g_new(FileData, 1);
   GList *node = g_list_alloc();

   f->filename = fname;
   f->nutebook_num = tab_num;

   node->data = f;

   if (filename_data == NULL)
      filename_data = node; // first file linked list node
   else
   {
      GList *list = filename_data;

      while (list != NULL)
      {
         if (((FileData *)list->data)->nutebook_num == tab_num)
         {
            ((FileData *)list->data)->filename = fname;
            break;
         }
         else
            list = g_list_next(list);
      }
   }
}

static gchar *notepad_get_filename(gint tab_num)
{
   GList *list = filename_data;

   while (list != NULL)
   {
      if (((FileData *)list->data)->nutebook_num == tab_num)
         return ((FileData *)list->data)->filename;
      else
         list = g_list_next(filename_data);
   }
   return NULL;
}

static void quit_application(GtkWidget *window, gpointer data)
{
   gtk_main_quit();
}
