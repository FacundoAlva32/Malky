#pragma once

#include <glib.h>
#include <stdbool.h>

typedef struct _MalkyDialog MalkyDialog;

typedef void (*MalkyInstallCb)(gboolean success, gpointer user_data);

gboolean malky_installer_check(const char *const *binary_names);

void malky_installer_ensure(const char *const *binary_names,
                            const char *const *package_names,
                            MalkyDialog *dialog,
                            MalkyInstallCb callback,
                            gpointer user_data,
                            GDestroyNotify user_data_free);
