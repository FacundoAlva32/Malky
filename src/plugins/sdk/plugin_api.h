#pragma once

#include <glib.h>
#include <stdbool.h>

typedef enum {
    M_PLUGIN_DND_UNKNOWN     = 0,
    M_PLUGIN_DND_YOUTUBE_URL = 1,
    M_PLUGIN_DND_IMAGE_PNG   = 2,
    M_PLUGIN_DND_IMAGE_JPEG  = 3,
    M_PLUGIN_DND_IMAGE_WEBP  = 4,
    M_PLUGIN_DND_IMAGE_AVIF  = 5,
    M_PLUGIN_DND_IMAGE       = 6,
    M_PLUGIN_DND_PDF         = 7,
    M_PLUGIN_DND_VIDEO       = 8,
    M_PLUGIN_DND_AUDIO       = 9,
    M_PLUGIN_DND_ZIP         = 10,
    M_PLUGIN_DND_TEXT        = 11,
} MalkyPluginDndType;

typedef struct {
    int      type;
    char    *uri;
    char    *path;
} MalkyPluginDndData;

typedef void (*MalkyPluginTaskCb)(int exit_status, gpointer user_data);
typedef void (*MalkyPluginChoiceCb)(int choice, gpointer user_data);
typedef void (*MalkyPluginInstallCb)(gboolean success, gpointer user_data);

typedef struct {
    void  (*dialog_say)(const char *text, int timeout_ms);
    void  (*dialog_ask)(const char *text,
                        const char *btn1, const char *btn2,
                        MalkyPluginChoiceCb callback,
                        gpointer user_data,
                        GDestroyNotify user_data_free);
    char *(*config_get)(const char *key, const char *def);
    void  (*task_spawn)(const char *const *argv,
                        MalkyPluginTaskCb on_done,
                        gpointer user_data);
    void  (*ensure_deps)(const char *const *binary_names,
                         const char *const *package_names,
                         MalkyPluginInstallCb callback,
                         gpointer user_data,
                         GDestroyNotify user_data_free);
} MalkyPluginAPI;

typedef struct {
    const char *name;
    const char *version;
    const char *description;
    const int   *dnd_types;
    bool       (*init)(const MalkyPluginAPI *api);
    void       (*shutdown)(void);
    bool       (*handle_dnd)(const MalkyPluginDndData *data);
} MalkyPluginInfo;
