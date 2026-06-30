#pragma once

#include <glib.h>
#include <cairo.h>

typedef struct _MalkyDialog MalkyDialog;

typedef void (*MalkyDialogCb)(int choice, gpointer user_data);

MalkyDialog *malky_dialog_new(void);
void         malky_dialog_free(MalkyDialog *dialog);

void     malky_dialog_say(MalkyDialog *dialog, const char *text, int timeout_ms);
void     malky_dialog_ask(MalkyDialog *dialog,
                          const char *text,
                          const char *btn1, const char *btn2,
                          MalkyDialogCb callback,
                          gpointer user_data,
                          GDestroyNotify user_data_free);
void     malky_dialog_clear(MalkyDialog *dialog);
void     malky_dialog_update(MalkyDialog *dialog, double dt);
void     malky_dialog_draw(MalkyDialog *dialog, cairo_t *cr,
                           double pet_cx, double pet_cy, double pet_radius);
gboolean malky_dialog_click(MalkyDialog *dialog, double x, double y);
gboolean malky_dialog_is_active(MalkyDialog *dialog);
int      malky_dialog_get_bubble_height(MalkyDialog *dialog);
