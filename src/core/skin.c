#include "skin.h"
#include "pet.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#define MAX_STATES M_PET_COUNT
#define DEFAULT_FRAME_W 128
#define DEFAULT_FRAME_H 128

typedef struct {
    int row;
    int frames;
    int fps;
} StateConfig;

struct _MalkySkin {
    cairo_surface_t *sheet;
    int              frame_w;
    int              frame_h;
    StateConfig      states[MAX_STATES];
};

static const char *state_names[] = {
    "idle", "walking", "sitting", "sleeping",
    "happy", "sad", "surprised",
};

static void
set_defaults(MalkySkin *skin)
{
    skin->frame_w = DEFAULT_FRAME_W;
    skin->frame_h = DEFAULT_FRAME_H;

    skin->states[M_PET_IDLE]      = (StateConfig){ .row = 0, .frames = 4, .fps = 4 };
    skin->states[M_PET_WALKING]   = (StateConfig){ .row = 1, .frames = 6, .fps = 10 };
    skin->states[M_PET_SITTING]   = (StateConfig){ .row = 2, .frames = 2, .fps = 2 };
    skin->states[M_PET_SLEEPING]  = (StateConfig){ .row = 3, .frames = 2, .fps = 1 };
    skin->states[M_PET_HAPPY]     = (StateConfig){ .row = 4, .frames = 4, .fps = 8 };
    skin->states[M_PET_SAD]       = (StateConfig){ .row = 5, .frames = 2, .fps = 2 };
    skin->states[M_PET_SURPRISED] = (StateConfig){ .row = 6, .frames = 2, .fps = 3 };
}

MalkySkin *
malky_skin_new(void)
{
    MalkySkin *skin = g_new0(MalkySkin, 1);
    set_defaults(skin);
    return skin;
}

void
malky_skin_free(MalkySkin *skin)
{
    if (!skin) return;
    g_clear_pointer(&skin->sheet, cairo_surface_destroy);
    g_free(skin);
}

gboolean
malky_skin_load(MalkySkin *skin, const char *dir)
{
    if (!skin || !dir) return FALSE;

    // Load optional skin.ini
    gchar *ini_path = g_build_filename(dir, "skin.ini", NULL);
    if (g_file_test(ini_path, G_FILE_TEST_EXISTS))
    {
        GKeyFile *kf = g_key_file_new();
        if (g_key_file_load_from_file(kf, ini_path, G_KEY_FILE_NONE, NULL))
        {
            GError *err = NULL;

            int w = g_key_file_get_integer(kf, "sprite", "width", &err);
            if (!err && w > 0) skin->frame_w = w;
            g_clear_error(&err);

            int h = g_key_file_get_integer(kf, "sprite", "height", &err);
            if (!err && h > 0) skin->frame_h = h;
            g_clear_error(&err);

            for (int i = 0; i < MAX_STATES; i++)
            {
                int row = g_key_file_get_integer(kf, state_names[i], "row", &err);
                if (!err) skin->states[i].row = row;
                g_clear_error(&err);

                int frames = g_key_file_get_integer(kf, state_names[i], "frames", &err);
                if (!err) skin->states[i].frames = frames;
                g_clear_error(&err);

                int fps = g_key_file_get_integer(kf, state_names[i], "fps", &err);
                if (!err) skin->states[i].fps = fps;
                g_clear_error(&err);
            }
        }
        g_key_file_free(kf);
    }
    g_free(ini_path);

    // Load sprite sheet (optional — fallback bird si no existe)
    gchar *png_path = g_build_filename(dir, "sheet.png", NULL);
    if (g_file_test(png_path, G_FILE_TEST_EXISTS))
    {
        cairo_surface_t *s = cairo_image_surface_create_from_png(png_path);
        if (cairo_surface_status(s) == CAIRO_STATUS_SUCCESS)
        {
            g_clear_pointer(&skin->sheet, cairo_surface_destroy);
            skin->sheet = s;
        }
        else
            cairo_surface_destroy(s);
    }
    g_free(png_path);

    return TRUE;
}

static void
draw_from_sheet(MalkySkin *skin, cairo_t *cr,
                int state, double anim_frame,
                double cx, double cy)
{
    if (state < 0 || state >= MAX_STATES)
        state = M_PET_IDLE;

    StateConfig *cfg = &skin->states[state];
    int frame = cfg->frames > 0
        ? ((int)(anim_frame * cfg->fps) % cfg->frames)
        : 0;

    double sx = frame * skin->frame_w;
    double sy = cfg->row * skin->frame_h;

    cairo_save(cr);
    cairo_translate(cr, cx - skin->frame_w / 2.0, cy - skin->frame_h / 2.0);
    cairo_set_source_surface(cr, skin->sheet, -sx, -sy);
    cairo_rectangle(cr, 0, 0, skin->frame_w, skin->frame_h);
    cairo_fill(cr);
    cairo_restore(cr);
}

// ── Fallback bird drawn with Cairo ─────────────────────────

static void
draw_fallback_bird(cairo_t *cr, int state, double anim_frame,
                   double cx, double cy)
{
    double r = 48.0;
    double bob = 0;
    double wing_angle = 0;
    double head_tilt = 0;
    double body_squash = 1.0;
    double eye_open = 1.0;

    switch (state)
    {
    case M_PET_IDLE:
        bob = 1.5 * sin(anim_frame * 2.0);
        eye_open = fmod(anim_frame, 3.5) < 0.1 ? 0.0 : 1.0;
        break;
    case M_PET_WALKING:
        bob = 2.0 * sin(anim_frame * 8.0);
        wing_angle = 0.35 * sin(anim_frame * 10.0);
        break;
    case M_PET_SITTING:
        body_squash = 0.92;
        eye_open = 0.3 + 0.3 * sin(anim_frame * 0.5);
        break;
    case M_PET_SLEEPING:
        body_squash = 0.90;
        eye_open = 0.0;
        head_tilt = -0.25;
        break;
    case M_PET_HAPPY:
        bob = 4.0 * sin(anim_frame * 10.0);
        wing_angle = 0.15 * sin(anim_frame * 14.0);
        break;
    case M_PET_SAD:
        body_squash = 0.95;
        head_tilt = -0.15;
        break;
    case M_PET_SURPRISED:
        eye_open = 1.3;
        wing_angle = 0.4;
        break;
    }

    cairo_save(cr);
    cairo_translate(cr, cx, cy + bob);

    // ── Tail ──
    cairo_set_source_rgba(cr, 0.25, 0.48, 0.80, 0.9);
    cairo_move_to(cr, -r * 0.42, -r * 0.10);
    cairo_line_to(cr, -r * 0.65, -r * 0.35);
    cairo_line_to(cr, -r * 0.55, -r * 0.05);
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_move_to(cr, -r * 0.42, 0);
    cairo_line_to(cr, -r * 0.70, -r * 0.10);
    cairo_line_to(cr, -r * 0.55, r * 0.10);
    cairo_close_path(cr);
    cairo_fill(cr);

    // ── Wing ──
    cairo_save(cr);
    cairo_translate(cr, -r * 0.05, -r * 0.10);
    cairo_rotate(cr, wing_angle);
    cairo_move_to(cr, -r * 0.05, r * 0.05);
    cairo_curve_to(cr, -r * 0.35, -r * 0.30, -r * 0.25, -r * 0.50, -r * 0.10, -r * 0.55);
    cairo_curve_to(cr, r * 0.05, -r * 0.50, r * 0.10, -r * 0.25, r * 0.05, -r * 0.10);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 0.28, 0.52, 0.85, 0.9);
    cairo_fill(cr);
    cairo_restore(cr);

    // ── Body ──
    cairo_save(cr);
    cairo_translate(cr, r * 0.05, r * 0.05);
    cairo_scale(cr, r * 0.45, r * 0.38 * body_squash);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * G_PI);
    cairo_set_source_rgba(cr, 0.35, 0.60, 0.95, 0.9);
    cairo_fill(cr);
    cairo_restore(cr);

    // ── Head ──
    double hx = r * 0.45;
    double hy = -r * 0.30 + head_tilt * r * 0.3;

    cairo_save(cr);
    cairo_translate(cr, hx, hy);
    cairo_scale(cr, 1.0, 0.85);
    cairo_arc(cr, 0, 0, r * 0.28, 0, 2 * G_PI);
    cairo_set_source_rgba(cr, 0.35, 0.60, 0.95, 0.9);
    cairo_fill(cr);
    cairo_restore(cr);

    // ── Beak ──
    cairo_move_to(cr, hx + r * 0.22, hy - r * 0.06);
    cairo_line_to(cr, hx + r * 0.50, hy + r * 0.02);
    cairo_line_to(cr, hx + r * 0.22, hy + r * 0.10);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 1.0, 0.60, 0.0, 0.9);
    cairo_fill(cr);

    // ── Eye ──
    double ex = hx + r * 0.10;
    double ey = hy - r * 0.08;

    if (eye_open > 0.1)
    {
        cairo_save(cr);
        cairo_translate(cr, ex, ey);
        cairo_scale(cr, 1.0, MIN(eye_open, 1.0));
        cairo_arc(cr, 0, 0, r * 0.08, 0, 2 * G_PI);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.95);
        cairo_fill(cr);
        cairo_restore(cr);

        if (eye_open > 0.3)
        {
            cairo_arc(cr, ex + r * 0.02, ey + r * 0.01, r * 0.04, 0, 2 * G_PI);
            cairo_set_source_rgba(cr, 0.05, 0.05, 0.10, 0.9);
            cairo_fill(cr);
        }
    }
    else
    {
        // Closed eye (line)
        cairo_set_source_rgba(cr, 0.05, 0.05, 0.10, 0.6);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, ex - r * 0.06, ey);
        cairo_line_to(cr, ex + r * 0.06, ey);
        cairo_stroke(cr);
    }

    // ── Legs (only when grounded) ──
    if (state == M_PET_IDLE || state == M_PET_SITTING ||
        state == M_PET_SLEEPING || state == M_PET_SAD)
    {
        double leg_swing = 0;
        if (state == M_PET_IDLE)
            leg_swing = 0.05 * sin(anim_frame * 1.5);

        cairo_set_source_rgba(cr, 0.25, 0.45, 0.75, 0.7);
        cairo_set_line_width(cr, 2.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

        cairo_move_to(cr, -r * 0.15, r * 0.38 + leg_swing);
        cairo_line_to(cr, -r * 0.15 + leg_swing * 10, r * 0.52);
        cairo_stroke(cr);

        cairo_move_to(cr, r * 0.15, r * 0.38 - leg_swing);
        cairo_line_to(cr, r * 0.15 - leg_swing * 10, r * 0.52);
        cairo_stroke(cr);

        cairo_set_line_width(cr, 1.0);
    }

    cairo_restore(cr);
}

void
malky_skin_draw(MalkySkin *skin, cairo_t *cr,
                int state, double anim_frame,
                double cx, double cy)
{
    if (skin && skin->sheet)
        draw_from_sheet(skin, cr, state, anim_frame, cx, cy);
    else
        draw_fallback_bird(cr, state, anim_frame, cx, cy);
}

// ── Skin scanning ──────────────────────────────────────────

static void
scan_skin_dir(GPtrArray *dirs, const char *base)
{
    GError *err = NULL;
    GDir *dir = g_dir_open(base, 0, &err);
    if (!dir)
    {
        g_clear_error(&err);
        return;
    }

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL)
    {
        gchar *full = g_build_filename(base, name, NULL);
        if (!g_file_test(full, G_FILE_TEST_IS_DIR))
        {
            g_free(full);
            continue;
        }

        gchar *ini = g_build_filename(full, "skin.ini", NULL);
        gchar *sheet = g_build_filename(full, "sheet.png", NULL);

        gboolean valid = g_file_test(ini, G_FILE_TEST_EXISTS)
                      || g_file_test(sheet, G_FILE_TEST_EXISTS);

        g_free(ini);
        g_free(sheet);

        if (valid)
            g_ptr_array_add(dirs, full);
        else
            g_free(full);
    }

    g_dir_close(dir);
}

char **
malky_skin_list_dirs(void)
{
    GPtrArray *dirs = g_ptr_array_new();

    gchar *user = g_build_filename(g_get_user_data_dir(), "malky", "skins", NULL);
    scan_skin_dir(dirs, user);
    g_free(user);

    gchar *exe = g_file_read_link("/proc/self/exe", NULL);
    if (exe)
    {
        gchar *exe_dir = g_path_get_dirname(exe);
        gchar *dev = g_build_filename(exe_dir, "skins", NULL);
        scan_skin_dir(dirs, dev);
        g_free(dev);
        g_free(exe_dir);
        g_free(exe);
    }

    g_ptr_array_add(dirs, NULL);
    return (char **)g_ptr_array_free(dirs, FALSE);
}

void
malky_skin_list_free(char **list)
{
    if (!list) return;
    for (int i = 0; list[i]; i++)
        g_free(list[i]);
    g_free(list);
}

char *
malky_skin_get_display_name(const char *dir)
{
    gchar *ini = g_build_filename(dir, "skin.ini", NULL);
    gchar *name = NULL;

    if (g_file_test(ini, G_FILE_TEST_EXISTS))
    {
        GKeyFile *kf = g_key_file_new();
        if (g_key_file_load_from_file(kf, ini, G_KEY_FILE_NONE, NULL))
        {
            gchar *n = g_key_file_get_string(kf, "info", "name", NULL);
            if (n)
                name = n;
        }
        g_key_file_free(kf);
    }
    g_free(ini);

    if (!name)
        name = g_path_get_basename(dir);

    return name;
}
