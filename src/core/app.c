#include "app.h"
#include "event.h"
#include "config.h"
#include "window.h"
#include "movement.h"
#include "dialog.h"
#include "pet.h"
#include "skin.h"
#include "dragdrop.h"
#include "plugin.h"
#include "context_menu.h"

#include <math.h>

#define TICK_INTERVAL_MS 33  // ~30 FPS
#define WIN_W 200
#define WIN_H 320

struct _MalkyApp {
    GtkApplication      *gtk_app;
    MalkyEventBus       *event_bus;
    MalkyConfig         *config;
    MalkyWindow         *window;
    MalkyMovement       *movement;
    MalkyDialog         *dialog;
    MalkyPet            *pet;
    MalkyDragDrop       *dragdrop;
    MalkyPluginManager  *plugin_mgr;
    MalkyContextMenu    *context_menu;
    guint                tick_source;
};

static void
on_draw(GtkDrawingArea *area, cairo_t *cr,
        int width, int height, gpointer user_data)
{
    (void)area;
    MalkyApp *app = (MalkyApp *)user_data;
    malky_pet_draw(app->pet, cr, width, height);
    if (malky_context_menu_is_visible(app->context_menu))
        malky_context_menu_draw(app->context_menu, cr, width, height);
}

static void
on_motion(GtkEventControllerMotion *controller,
          double x, double y, gpointer user_data)
{
    (void)controller;
    MalkyApp *app = (MalkyApp *)user_data;
    if (!malky_context_menu_is_visible(app->context_menu))
        return;
    malky_context_menu_set_mouse_pos(app->context_menu, x, y);
    malky_window_queue_redraw(app->window);
}

// ── Skin picker ────────────────────────────────────────────

typedef struct {
    MalkyApp *app;
    char    **dirs;
    int       count;
    int       offset;
} SkinPicker;

static void
skin_picker_free(SkinPicker *sp)
{
    if (!sp) return;
    malky_skin_list_free(sp->dirs);
    g_free(sp);
}

static void
show_skin_batch(SkinPicker *sp);

static void
on_skin_choice(int choice, gpointer user_data)
{
    SkinPicker *sp = (SkinPicker *)user_data;
    if (choice < 0) { skin_picker_free(sp); return; }

    if (choice == 0)
    {
        malky_pet_load_skin_dir(sp->app->pet, sp->dirs[sp->offset]);
        malky_pet_say(sp->app->pet, "Skin cambiado!", 1500);
        skin_picker_free(sp);
        return;
    }

    // choice == 1
    int next = sp->offset + 2;
    if (next < sp->count)
    {
        // "Mas..." → show next batch
        sp->offset = next;
        show_skin_batch(sp);
    }
    else if (sp->offset + 1 < sp->count)
    {
        // Last pair: second skin was chosen
        malky_pet_load_skin_dir(sp->app->pet, sp->dirs[sp->offset + 1]);
        malky_pet_say(sp->app->pet, "Skin cambiado!", 1500);
        skin_picker_free(sp);
    }
    else
    {
        // Only one skin shown + "Cancelar" → cancelled
        skin_picker_free(sp);
    }
}

static void
show_skin_batch(SkinPicker *sp)
{
    if (sp->offset >= sp->count)
    {
        malky_pet_say(sp->app->pet, "No hay mas skins.", 2000);
        skin_picker_free(sp);
        return;
    }

    char *name1 = malky_skin_get_display_name(sp->dirs[sp->offset]);

    if (sp->offset + 1 < sp->count)
    {
        char *name2 = malky_skin_get_display_name(sp->dirs[sp->offset + 1]);
        gboolean more = (sp->offset + 2 < sp->count);
        const char *btn2 = more ? "Mas..." : name2;

        malky_dialog_ask(sp->app->dialog,
            "Elegi un skin:", name1, btn2,
            on_skin_choice, sp, NULL);

        g_free(name1);
        g_free(name2);
    }
    else
    {
        malky_dialog_ask(sp->app->dialog,
            "Elegi un skin:", name1, "Cancelar",
            on_skin_choice, sp, NULL);
        g_free(name1);
    }
}

static void
start_skin_picker(MalkyApp *app)
{
    SkinPicker *sp = g_new0(SkinPicker, 1);
    sp->app = app;
    sp->dirs = malky_skin_list_dirs();
    sp->count = 0;

    if (sp->dirs)
        while (sp->dirs[sp->count]) sp->count++;

    if (sp->count == 0)
    {
        malky_pet_say(app->pet, "No hay skins instaladas.", 2500);
        malky_skin_list_free(sp->dirs);
        g_free(sp);
        return;
    }

    show_skin_batch(sp);
}

static void
on_menu_action(MalkyMenuAction action, gpointer user_data)
{
    MalkyApp *app = (MalkyApp *)user_data;

    switch (action) {
        case M_MENU_SLEEP_TOGGLE:
        {
            MalkyPetState cur = malky_pet_get_state(app->pet);
            if (cur == M_PET_SLEEPING) {
                malky_pet_set_state(app->pet, M_PET_IDLE);
                malky_context_menu_set_sleeping(app->context_menu, FALSE);
                malky_pet_say(app->pet, "Buenos dias!", 2000);
            } else {
                malky_pet_say(app->pet, "Me voy a dormir...", 2000);
                malky_pet_set_state(app->pet, M_PET_SLEEPING);
                malky_context_menu_set_sleeping(app->context_menu, TRUE);
            }
            break;
        }
        case M_MENU_SKIN:
            start_skin_picker(app);
            break;
        case M_MENU_PLUGINS:
            malky_pet_say(app->pet,
                "Gestion de plugins proximamente!", 3000);
            break;
        case M_MENU_QUIT:
        {
            MalkyEvent ev = { .type = M_EVT_QUIT };
            malky_event_publish(app->event_bus, &ev);
            break;
        }
    }
}

static void
on_click(GtkGestureClick *gesture, int n_press,
         double x, double y, gpointer user_data)
{
    (void)n_press;
    MalkyApp *app = (MalkyApp *)user_data;

    guint button = gtk_gesture_single_get_current_button(
        GTK_GESTURE_SINGLE(gesture));

    if (button == 3) {
        // Right click → show context menu at click position
        malky_context_menu_show(app->context_menu, x, y,
                                on_menu_action, app);
        malky_window_queue_redraw(app->window);
        return;
    }

    // Left click (or any other button)
    if (malky_context_menu_is_visible(app->context_menu)) {
        if (malky_context_menu_click(app->context_menu, x, y))
            return;
        malky_context_menu_hide(app->context_menu);
        malky_window_queue_redraw(app->window);
        return;
    }

    if (malky_dialog_click(app->dialog, x, y))
        return;

    malky_pet_click(app->pet);
}

static void
on_window_mapped(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    MalkyApp *app = (MalkyApp *)user_data;

    GdkDisplay *display = gdk_display_get_default();
    int mw = 1920, mh = 1080;

    if (display) {
        GListModel *monitors = gdk_display_get_monitors(display);
        guint n = g_list_model_get_n_items(monitors);
        if (n > 0) {
            GdkMonitor *monitor = GDK_MONITOR(
                g_list_model_get_item(monitors, 0));
            if (monitor) {
                GdkRectangle geo;
                gdk_monitor_get_geometry(monitor, &geo);
                mw = geo.width;
                mh = geo.height;
                g_object_unref(monitor);
            }
        }
    }

    // Margins so pet doesn't go off-screen
    double margin = 100;
    malky_movement_set_bounds(app->movement,
        margin, margin,
        mw - margin * 2 - WIN_W,
        mh - margin * 2 - WIN_H);

    // Start in center-ish
    malky_movement_set_position(app->movement, mw / 2.0, mh / 2.0);
    malky_movement_pick_random_target(app->movement);
}

static gboolean
on_tick(gpointer user_data)
{
    MalkyApp *app = (MalkyApp *)user_data;
    double dt = TICK_INTERVAL_MS / 1000.0;

    malky_pet_update(app->pet, dt);

    double px, py;
    malky_movement_get_position(app->movement, &px, &py);
    malky_window_move(app->window, (int)(px - WIN_W / 2),
                      (int)(py - WIN_H / 2));
    malky_window_queue_redraw(app->window);

    return G_SOURCE_CONTINUE;
}

static void
on_activate(GtkApplication *gtk_app, gpointer user_data)
{
    (void)gtk_app;
    MalkyApp *app = (MalkyApp *)user_data;

    app->window = malky_window_new();
    gtk_window_set_application(
        GTK_WINDOW(malky_window_get_widget(app->window)),
        app->gtk_app);

    // Draw area
    gtk_drawing_area_set_draw_func(
        GTK_DRAWING_AREA(malky_window_get_draw_area(app->window)),
        on_draw, app, NULL);

    // Drag & Drop
    malky_dragdrop_attach(app->dragdrop,
                          malky_window_get_widget(app->window));

    // Click gesture
    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
    g_signal_connect(click, "pressed", G_CALLBACK(on_click), app);
    gtk_widget_add_controller(
        malky_window_get_widget(app->window),
        GTK_EVENT_CONTROLLER(click));

    // Motion controller (for hover on context menu)
    GtkEventController *motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "motion", G_CALLBACK(on_motion), app);
    gtk_widget_add_controller(
        malky_window_get_widget(app->window),
        motion);

    // Wait for window to be mapped before getting monitor info
    g_signal_connect(malky_window_get_widget(app->window), "map",
                     G_CALLBACK(on_window_mapped), app);

    gtk_widget_set_visible(malky_window_get_widget(app->window), TRUE);

    // Start tick timer
    app->tick_source = g_timeout_add(TICK_INTERVAL_MS, on_tick, app);
}

static gboolean
on_dnd_event(MalkyEvent *ev, gpointer user_data)
{
    MalkyApp *app = (MalkyApp *)user_data;
    if (ev->type != M_EVT_DND_RECEIVED || !ev->data)
        return FALSE;

    MalkyDndData *data = (MalkyDndData *)ev->data;
    malky_pet_handle_dnd(app->pet, data->type,
                         data->path ? data->path : data->uri);
    return FALSE;
}

static gboolean
on_quit_event(MalkyEvent *ev, gpointer user_data)
{
    (void)ev;
    MalkyApp *app = (MalkyApp *)user_data;
    g_application_quit(G_APPLICATION(app->gtk_app));
    return TRUE;
}

MalkyApp *
malky_app_new(void)
{
    MalkyApp *app = g_new0(MalkyApp, 1);

    app->event_bus = malky_event_bus_new();
    app->config    = malky_config_new(app->event_bus);

    malky_event_subscribe(app->event_bus, M_EVT_QUIT,
                          on_quit_event, app, NULL);

    // Create movement, dialog, pet
    app->movement = malky_movement_new();
    app->dialog   = malky_dialog_new();
    app->pet      = malky_pet_new(app->movement, app->dialog);

    app->dragdrop = malky_dragdrop_new(app->event_bus);

    app->context_menu = malky_context_menu_new();

    malky_event_subscribe(app->event_bus, M_EVT_DND_RECEIVED,
                          on_dnd_event, app, NULL);

    // Plugin system
    app->plugin_mgr = malky_plugin_manager_new(app->event_bus,
                                                app->dialog,
                                                app->config);

    const char *home = g_get_home_dir();
    gchar *plugdir = g_build_filename(home, ".local", "lib",
                                      "malky", "plugins", NULL);
    malky_plugin_manager_scan(app->plugin_mgr, plugdir);
    g_free(plugdir);

    // Also scan relative to the executable (dev build)
    gchar *exe = g_file_read_link("/proc/self/exe", NULL);
    if (exe)
    {
        gchar *exe_dir = g_path_get_dirname(exe);
        gchar *abs = g_build_filename(exe_dir, "plugins", NULL);
        malky_plugin_manager_scan(app->plugin_mgr, abs);
        g_free(abs);
        g_free(exe_dir);
        g_free(exe);
    }

    app->gtk_app = gtk_application_new("com.malky.Malky",
                                        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app->gtk_app, "activate",
                     G_CALLBACK(on_activate), app);

    return app;
}

int
malky_app_run(MalkyApp *app)
{
    int ret = g_application_run(G_APPLICATION(app->gtk_app), 0, NULL);
    return ret;
}

void
malky_app_free(MalkyApp *app)
{
    if (!app) return;

    if (app->tick_source)
        g_source_remove(app->tick_source);

    if (app->pet)       malky_pet_free(app->pet);
    if (app->dialog)    malky_dialog_free(app->dialog);
    if (app->movement)  malky_movement_free(app->movement);
    if (app->dragdrop)  malky_dragdrop_free(app->dragdrop);
    if (app->context_menu) malky_context_menu_free(app->context_menu);
    if (app->plugin_mgr) malky_plugin_manager_free(app->plugin_mgr);
    if (app->window)    malky_window_free(app->window);
    if (app->config)   malky_config_free(app->config);
    if (app->event_bus) malky_event_bus_free(app->event_bus);
    g_object_unref(app->gtk_app);
    g_free(app);
}

MalkyEventBus *
malky_app_get_event_bus(MalkyApp *app)
{
    return app->event_bus;
}

MalkyConfig *
malky_app_get_config(MalkyApp *app)
{
    return app->config;
}

MalkyWindow *
malky_app_get_window(MalkyApp *app)
{
    return app->window;
}

MalkyPet *
malky_app_get_pet(MalkyApp *app)
{
    return app->pet;
}
