#include "window.h"

struct _MalkyWindow {
    GtkWidget *widget;
    GtkWidget *draw_area;
};

static void
apply_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider,
        "window { background: transparent; }");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

MalkyWindow *
malky_window_new(void)
{
    MalkyWindow *win = g_new0(MalkyWindow, 1);

    win->widget = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win->widget), "Malky");
    gtk_window_set_default_size(GTK_WINDOW(win->widget), 200, 320);
    gtk_window_set_resizable(GTK_WINDOW(win->widget), TRUE);
    gtk_window_set_decorated(GTK_WINDOW(win->widget), FALSE);

    apply_css();

    win->draw_area = gtk_drawing_area_new();
    gtk_widget_set_vexpand(win->draw_area, TRUE);
    gtk_widget_set_hexpand(win->draw_area, TRUE);
    gtk_window_set_child(GTK_WINDOW(win->widget), win->draw_area);

    return win;
}

void
malky_window_free(MalkyWindow *window)
{
    if (!window) return;
    if (window->widget)
        gtk_window_destroy(GTK_WINDOW(window->widget));
    g_free(window);
}

GtkWidget *
malky_window_get_widget(MalkyWindow *window)
{
    return window->widget;
}

GtkWidget *
malky_window_get_draw_area(MalkyWindow *window)
{
    return window->draw_area;
}

void
malky_window_queue_redraw(MalkyWindow *window)
{
    gtk_widget_queue_draw(window->draw_area);
}

void
malky_window_get_size(MalkyWindow *window, int *w, int *h)
{
    if (w) *w = gtk_widget_get_width(window->draw_area);
    if (h) *h = gtk_widget_get_height(window->draw_area);
}

void
malky_window_move(MalkyWindow *window, int x, int y)
{
    GdkSurface *surface = gtk_native_get_surface(
        GTK_NATIVE(window->widget));
    if (!surface) return;

    GdkToplevel *toplevel = GDK_TOPLEVEL(surface);
    if (!toplevel) return;

    if (gdk_toplevel_supports_edge_constraints(toplevel)) {
        GdkToplevelLayout *layout = gdk_toplevel_layout_new();
        gdk_toplevel_layout_set_resizable(layout, FALSE);
        gdk_toplevel_present(toplevel, layout);
        gdk_toplevel_layout_unref(layout);
    }
    (void)x;
    (void)y;
}
