#pragma once

#include <gtk/gtk.h>

typedef struct _MalkyWindow MalkyWindow;

MalkyWindow *malky_window_new(void);
void         malky_window_free(MalkyWindow *window);

GtkWidget *malky_window_get_widget(MalkyWindow *window);
GtkWidget *malky_window_get_draw_area(MalkyWindow *window);
void       malky_window_queue_redraw(MalkyWindow *window);
void       malky_window_get_size(MalkyWindow *window, int *w, int *h);
void       malky_window_move(MalkyWindow *window, int x, int y);
