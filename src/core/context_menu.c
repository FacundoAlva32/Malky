#include "context_menu.h"

#include <math.h>
#include <string.h>

#define MENU_W 120
#define ITEM_H 28
#define PAD 6
#define ROUND_R 6

struct _MalkyContextMenu {
    gboolean    visible;
    gboolean    sleeping;
    double      anchor_x;
    double      anchor_y;
    double      item_rects[4][4];
    int         hover_index;
    double      mouse_x;
    double      mouse_y;
    gboolean    mouse_valid;
    MalkyMenuCb callback;
    gpointer    user_data;
};

static const char *
item_label(MalkyMenuAction action, gboolean sleeping)
{
    switch (action) {
        case M_MENU_SLEEP_TOGGLE:
            return sleeping ? "Despertar" : "Dormir";
        case M_MENU_SKIN:     return "Apariencia";
        case M_MENU_PLUGINS:  return "Plugins";
        case M_MENU_QUIT:     return "Salir";
    }
    return "";
}

static void
rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r)
{
    cairo_move_to(cr, x + r, y);
    cairo_arc(cr, x + w - r, y + r, r, -G_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, G_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, G_PI / 2, G_PI);
    cairo_arc(cr, x + r, y + r, r, G_PI, 3 * G_PI / 2);
    cairo_close_path(cr);
}

MalkyContextMenu *
malky_context_menu_new(void)
{
    MalkyContextMenu *menu = g_new0(MalkyContextMenu, 1);
    menu->hover_index = -1;
    return menu;
}

void
malky_context_menu_free(MalkyContextMenu *menu)
{
    g_free(menu);
}

void
malky_context_menu_show(MalkyContextMenu *menu,
                        double anchor_x, double anchor_y,
                        MalkyMenuCb callback,
                        gpointer user_data)
{
    menu->visible = TRUE;
    menu->anchor_x = anchor_x;
    menu->anchor_y = anchor_y;
    menu->callback = callback;
    menu->user_data = user_data;
    menu->hover_index = -1;
    menu->mouse_valid = FALSE;
}

void
malky_context_menu_set_mouse_pos(MalkyContextMenu *menu, double x, double y)
{
    menu->mouse_x = x;
    menu->mouse_y = y;
    menu->mouse_valid = TRUE;
}

void
malky_context_menu_hide(MalkyContextMenu *menu)
{
    menu->visible = FALSE;
    menu->hover_index = -1;
}

gboolean
malky_context_menu_is_visible(MalkyContextMenu *menu)
{
    return menu->visible;
}

void
malky_context_menu_set_sleeping(MalkyContextMenu *menu, gboolean sleeping)
{
    menu->sleeping = sleeping;
}

gboolean
malky_context_menu_click(MalkyContextMenu *menu, double x, double y)
{
    if (!menu->visible)
        return FALSE;

    for (int i = 0; i < 4; i++)
    {
        double rx = menu->item_rects[i][0];
        double ry = menu->item_rects[i][1];
        double rw = menu->item_rects[i][2];
        double rh = menu->item_rects[i][3];

        if (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh)
        {
            MalkyMenuAction action = (MalkyMenuAction)i;
            MalkyMenuCb cb = menu->callback;
            gpointer data = menu->user_data;
            menu->visible = FALSE;
            menu->callback = NULL;
            if (cb)
                cb(action, data);
            return TRUE;
        }
    }

    return FALSE;
}

void
malky_context_menu_draw(MalkyContextMenu *menu, cairo_t *cr,
                        int win_w, int win_h)
{
    if (!menu->visible)
        return;

    cairo_save(cr);

    int n_items = 4;
    double menu_h = n_items * ITEM_H + PAD * 2;

    double mx = menu->anchor_x - MENU_W / 2.0;
    double my = menu->anchor_y - menu_h / 2.0;

    mx = CLAMP(mx, 4, win_w - MENU_W - 4);
    my = CLAMP(my, 4, win_h - menu_h - 4);

    // Shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.12);
    rounded_rect(cr, mx + 3, my + 3, MENU_W, menu_h, ROUND_R);
    cairo_fill(cr);

    // Background
    cairo_set_source_rgba(cr, 1, 1, 1, 0.95);
    rounded_rect(cr, mx, my, MENU_W, menu_h, ROUND_R);
    cairo_fill(cr);

    // Border
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.35, 0.25);
    cairo_set_line_width(cr, 1.0);
    rounded_rect(cr, mx, my, MENU_W, menu_h, ROUND_R);
    cairo_stroke(cr);

    for (int i = 0; i < n_items; i++)
    {
        double iy = my + PAD + i * ITEM_H;
        double iw = MENU_W - PAD * 2;

        menu->item_rects[i][0] = mx + PAD;
        menu->item_rects[i][1] = iy;
        menu->item_rects[i][2] = iw;
        menu->item_rects[i][3] = ITEM_H;
    }

    menu->hover_index = -1;
    if (menu->mouse_valid)
    {
        for (int i = 0; i < n_items; i++)
        {
            double rx = menu->item_rects[i][0];
            double ry = menu->item_rects[i][1];
            double rw = menu->item_rects[i][2];
            double rh = menu->item_rects[i][3];
            if (menu->mouse_x >= rx && menu->mouse_x <= rx + rw &&
                menu->mouse_y >= ry && menu->mouse_y <= ry + rh)
            {
                menu->hover_index = i;
                break;
            }
        }
    }

    for (int i = 0; i < n_items; i++)
    {
        double iy = my + PAD + i * ITEM_H;
        double iw = MENU_W - PAD * 2;

        // Hover highlight
        if (i == menu->hover_index)
        {
            cairo_set_source_rgba(cr, 0.35, 0.60, 0.95, 0.20);
            rounded_rect(cr, mx + PAD, iy, iw, ITEM_H, 5);
            cairo_fill(cr);
        }

        // Separator
        if (i > 0)
        {
            cairo_set_source_rgba(cr, 0.3, 0.3, 0.35, 0.12);
            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, mx + PAD + 4, iy);
            cairo_line_to(cr, mx + MENU_W - PAD - 4, iy);
            cairo_stroke(cr);
        }

        // Text
        cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.9);
        cairo_select_font_face(cr, "sans-serif",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 11);

        const char *label = item_label((MalkyMenuAction)i, menu->sleeping);
        cairo_text_extents_t te;
        cairo_text_extents(cr, label, &te);
        cairo_move_to(cr,
            mx + PAD + (iw - te.width) / 2,
            iy + (ITEM_H + te.height) / 2);
        cairo_show_text(cr, label);
    }

    cairo_restore(cr);
}
