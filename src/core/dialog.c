#include "dialog.h"

#include <string.h>
#include <math.h>
#include <pango/pangocairo.h>

#define TYPING_INTERVAL 0.04
#define MAX_BUBBLE_W 180
#define PAD_X 10
#define PAD_Y 6
#define BTN_H 28
#define BTN_R 6
#define BTN_SPACING 6
#define BTN_PAD_TOP 4

struct _MalkyDialog {
    char     *full_text;
    char     *visible_text;
    int       visible_cap;
    int       char_index;
    double    char_timer;
    double    timeout;
    double    elapsed;
    gboolean  active;
    int       bubble_h;

    gboolean  interactive;
    char      btn1[64];
    char      btn2[64];
    MalkyDialogCb callback;
    gpointer  cb_data;
    GDestroyNotify cb_data_free;

    double    draw_bx;
    double    draw_by;
    double    draw_bw;
    double    draw_bh;
    double    btn1_x, btn1_y, btn1_w, btn1_h;
    double    btn2_x, btn2_y, btn2_w, btn2_h;
};

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

MalkyDialog *
malky_dialog_new(void)
{
    MalkyDialog *dialog = g_new0(MalkyDialog, 1);
    dialog->visible_cap = 128;
    dialog->visible_text = g_new0(char, dialog->visible_cap);
    return dialog;
}

void
malky_dialog_free(MalkyDialog *dialog)
{
    if (!dialog) return;
    g_free(dialog->full_text);
    g_free(dialog->visible_text);
    if (dialog->cb_data_free && dialog->cb_data)
        dialog->cb_data_free(dialog->cb_data);
    g_free(dialog);
}

static void
cleanup_interactive(MalkyDialog *dialog)
{
    dialog->interactive = FALSE;
    dialog->callback = NULL;
    if (dialog->cb_data_free && dialog->cb_data)
    {
        dialog->cb_data_free(dialog->cb_data);
        dialog->cb_data = NULL;
    }
    dialog->cb_data_free = NULL;
}

static double
measure_bubble_height(MalkyDialog *dialog)
{
    if (!dialog->full_text)
        return 0;

    cairo_surface_t *tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t *cr = cairo_create(tmp);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, dialog->full_text, -1);

    int text_w, text_h;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    double bw = text_w + PAD_X * 2;
    double bh = text_h + PAD_Y * 2;

    if (bw > MAX_BUBBLE_W)
    {
        pango_layout_set_width(layout, (MAX_BUBBLE_W - PAD_X * 2) * PANGO_SCALE);
        pango_layout_get_pixel_size(layout, &text_w, &text_h);
        bh = text_h + PAD_Y * 2;
    }

    if (dialog->interactive)
        bh += BTN_PAD_TOP + BTN_H + PAD_Y;

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(tmp);

    return bh;
}

void
malky_dialog_say(MalkyDialog *dialog, const char *text, int timeout_ms)
{
    cleanup_interactive(dialog);

    g_free(dialog->full_text);
    dialog->full_text = g_strdup(text);
    dialog->char_index = 0;
    dialog->char_timer = 0;
    dialog->visible_text[0] = '\0';
    dialog->timeout = timeout_ms / 1000.0;
    dialog->elapsed = 0;
    dialog->active = TRUE;
    dialog->bubble_h = (int)measure_bubble_height(dialog);
}

void
malky_dialog_ask(MalkyDialog *dialog,
                 const char *text,
                 const char *btn1, const char *btn2,
                 MalkyDialogCb callback,
                 gpointer user_data,
                 GDestroyNotify user_data_free)
{
    cleanup_interactive(dialog);

    g_free(dialog->full_text);
    dialog->full_text = g_strdup(text);
    dialog->char_index = 0;
    dialog->char_timer = 0;
    dialog->visible_text[0] = '\0';
    dialog->timeout = 0;
    dialog->elapsed = 0;
    dialog->active = TRUE;
    dialog->interactive = TRUE;
    dialog->callback = callback;
    dialog->cb_data = user_data;
    dialog->cb_data_free = user_data_free;

    g_strlcpy(dialog->btn1, btn1 ? btn1 : "Si", sizeof(dialog->btn1));
    g_strlcpy(dialog->btn2, btn2 ? btn2 : "No", sizeof(dialog->btn2));

    dialog->bubble_h = (int)measure_bubble_height(dialog);
}

void
malky_dialog_clear(MalkyDialog *dialog)
{
    if (dialog->interactive && dialog->callback)
        dialog->callback(-1, dialog->cb_data);

    cleanup_interactive(dialog);
    dialog->active = FALSE;
    g_clear_pointer(&dialog->full_text, g_free);
    dialog->visible_text[0] = '\0';
    dialog->char_index = 0;
}

void
malky_dialog_update(MalkyDialog *dialog, double dt)
{
    if (!dialog->active) return;

    dialog->elapsed += dt;

    if (!dialog->interactive
        && dialog->timeout > 0
        && dialog->elapsed >= dialog->timeout)
    {
        malky_dialog_clear(dialog);
        return;
    }

    if (!dialog->full_text) return;

    dialog->char_timer += dt;
    if (dialog->char_timer >= TYPING_INTERVAL)
    {
        dialog->char_timer = 0;
        int len = strlen(dialog->full_text);
        if (dialog->char_index < len)
        {
            int copy_len = MIN(dialog->char_index + 1, dialog->visible_cap - 1);
            strncpy(dialog->visible_text, dialog->full_text, copy_len);
            dialog->visible_text[copy_len] = '\0';
            dialog->char_index++;
        }
    }
}

void
malky_dialog_draw(MalkyDialog *dialog, cairo_t *cr,
                  double pet_cx, double pet_cy, double pet_radius)
{
    if (!dialog->active || !dialog->visible_text[0]) return;

    cairo_save(cr);

    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, dialog->visible_text, -1);

    int text_w, text_h;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    double bubble_w = text_w + PAD_X * 2;
    double bubble_h = text_h + PAD_Y * 2;

    if (dialog->interactive)
        bubble_h += BTN_PAD_TOP + BTN_H + PAD_Y;

    if (bubble_w > MAX_BUBBLE_W)
    {
        pango_layout_set_width(layout, (MAX_BUBBLE_W - PAD_X * 2) * PANGO_SCALE);
        pango_layout_get_pixel_size(layout, &text_w, &text_h);
        bubble_w = MAX_BUBBLE_W;

        bubble_h = text_h + PAD_Y * 2;
        if (dialog->interactive)
            bubble_h += BTN_PAD_TOP + BTN_H + PAD_Y;
    }

    double tail_size = 8;

    double bx = pet_cx - bubble_w / 2;
    bx = CLAMP(bx, 2, (pet_cx * 2) - bubble_w - 2);

    double by = pet_cy - pet_radius - bubble_h - tail_size;
    by = MAX(by, 2);

    // Store for hit testing
    dialog->draw_bx = bx;
    dialog->draw_by = by;
    dialog->draw_bw = bubble_w;
    dialog->draw_bh = bubble_h;

    // Bubble background
    cairo_set_source_rgba(cr, 1, 1, 1, 0.92);
    rounded_rect(cr, bx, by, bubble_w, bubble_h, 8);
    cairo_fill(cr);

    // Bubble border
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.35, 0.3);
    cairo_set_line_width(cr, 1.0);
    rounded_rect(cr, bx, by, bubble_w, bubble_h, 8);
    cairo_stroke(cr);

    // Tail (always points down from bubble toward the pet)
    double tail_x = CLAMP(pet_cx, bx + 10, bx + bubble_w - 10);
    double tail_y = by + bubble_h;
    cairo_move_to(cr, tail_x - 6, tail_y);
    cairo_line_to(cr, tail_x, tail_y + tail_size);
    cairo_line_to(cr, tail_x + 6, tail_y);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.92);
    cairo_fill(cr);

    // Text
    cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.9);
    cairo_move_to(cr, bx + PAD_X, by + PAD_Y);
    pango_cairo_show_layout(cr, layout);

    // Buttons
    if (dialog->interactive)
    {
        double btn_y = by + PAD_Y + text_h + BTN_PAD_TOP;
        double avail_w = bubble_w - PAD_X * 2;
        double btn_w = (avail_w - BTN_SPACING) / 2.0;
        double btn1_x = bx + PAD_X;
        double btn2_x = bx + PAD_X + btn_w + BTN_SPACING;

        dialog->btn1_x = btn1_x;
        dialog->btn1_y = btn_y;
        dialog->btn1_w = btn_w;
        dialog->btn1_h = BTN_H;

        dialog->btn2_x = btn2_x;
        dialog->btn2_y = btn_y;
        dialog->btn2_w = btn_w;
        dialog->btn2_h = BTN_H;

        // Button 1 background
        cairo_set_source_rgba(cr, 0.35, 0.60, 0.95, 0.85);
        rounded_rect(cr, btn1_x, btn_y, btn_w, BTN_H, BTN_R);
        cairo_fill(cr);

        // Button 1 text
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_select_font_face(cr, "sans-serif",
                               CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12);
        cairo_text_extents_t te;
        cairo_text_extents(cr, dialog->btn1, &te);
        cairo_move_to(cr,
            btn1_x + (btn_w - te.width) / 2,
            btn_y + (BTN_H + te.height) / 2);
        cairo_show_text(cr, dialog->btn1);

        // Button 2 background
        cairo_set_source_rgba(cr, 0.35, 0.60, 0.95, 0.85);
        rounded_rect(cr, btn2_x, btn_y, btn_w, BTN_H, BTN_R);
        cairo_fill(cr);

        // Button 2 text
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_text_extents(cr, dialog->btn2, &te);
        cairo_move_to(cr,
            btn2_x + (btn_w - te.width) / 2,
            btn_y + (BTN_H + te.height) / 2);
        cairo_show_text(cr, dialog->btn2);
    }

    g_object_unref(layout);
    cairo_restore(cr);
}

gboolean
malky_dialog_click(MalkyDialog *dialog, double x, double y)
{
    if (!dialog->active || !dialog->interactive)
        return FALSE;

    if (x >= dialog->btn1_x && x <= dialog->btn1_x + dialog->btn1_w
        && y >= dialog->btn1_y && y <= dialog->btn1_y + dialog->btn1_h)
    {
        MalkyDialogCb cb = dialog->callback;
        gpointer data = dialog->cb_data;
        GDestroyNotify free = dialog->cb_data_free;
        dialog->callback = NULL;
        dialog->cb_data = NULL;
        dialog->cb_data_free = NULL;
        malky_dialog_clear(dialog);
        if (cb) cb(0, data);
        if (free) free(data);
        return TRUE;
    }

    if (x >= dialog->btn2_x && x <= dialog->btn2_x + dialog->btn2_w
        && y >= dialog->btn2_y && y <= dialog->btn2_y + dialog->btn2_h)
    {
        MalkyDialogCb cb = dialog->callback;
        gpointer data = dialog->cb_data;
        GDestroyNotify free = dialog->cb_data_free;
        dialog->callback = NULL;
        dialog->cb_data = NULL;
        dialog->cb_data_free = NULL;
        malky_dialog_clear(dialog);
        if (cb) cb(1, data);
        if (free) free(data);
        return TRUE;
    }

    return FALSE;
}

gboolean
malky_dialog_is_active(MalkyDialog *dialog)
{
    return dialog->active;
}

int
malky_dialog_get_bubble_height(MalkyDialog *dialog)
{
    return dialog->active ? dialog->bubble_h : 0;
}
