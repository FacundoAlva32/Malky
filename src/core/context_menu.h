#pragma once

#include <glib.h>
#include <cairo.h>

typedef struct _MalkyContextMenu MalkyContextMenu;

typedef enum {
    M_MENU_SLEEP_TOGGLE,
    M_MENU_CONFIG,
    M_MENU_PLUGINS,
    M_MENU_QUIT,
} MalkyMenuAction;

typedef void (*MalkyMenuCb)(MalkyMenuAction action, gpointer user_data);

MalkyContextMenu *malky_context_menu_new(void);
void              malky_context_menu_free(MalkyContextMenu *menu);

void     malky_context_menu_show(MalkyContextMenu *menu,
                                 double anchor_x, double anchor_y,
                                 MalkyMenuCb callback,
                                 gpointer user_data);
void     malky_context_menu_hide(MalkyContextMenu *menu);
gboolean malky_context_menu_is_visible(MalkyContextMenu *menu);
gboolean malky_context_menu_click(MalkyContextMenu *menu, double x, double y);
void     malky_context_menu_draw(MalkyContextMenu *menu, cairo_t *cr,
                                 int win_w, int win_h);
void     malky_context_menu_set_sleeping(MalkyContextMenu *menu,
                                         gboolean sleeping);
void     malky_context_menu_set_mouse_pos(MalkyContextMenu *menu,
                                          double x, double y);
