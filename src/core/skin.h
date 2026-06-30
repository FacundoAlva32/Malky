#pragma once

#include <glib.h>
#include <cairo.h>

typedef struct _MalkySkin MalkySkin;

MalkySkin *malky_skin_new(void);
void       malky_skin_free(MalkySkin *skin);

gboolean   malky_skin_load(MalkySkin *skin, const char *dir);
void       malky_skin_draw(MalkySkin *skin, cairo_t *cr,
                           int state, double anim_frame,
                           double cx, double cy);

char     **malky_skin_list_dirs(void);
void       malky_skin_list_free(char **list);
char      *malky_skin_get_display_name(const char *dir);
