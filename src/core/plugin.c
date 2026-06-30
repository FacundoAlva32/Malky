#include "plugin.h"
#include "event.h"
#include "dialog.h"
#include "config.h"
#include "dragdrop.h"
#include "installer.h"
#include "plugin_api.h"

#include <gmodule.h>
#include <sys/wait.h>

typedef struct {
    GModule         *handle;
    MalkyPluginInfo *info;
    gchar           *path;
} PluginEntry;

struct _MalkyPluginManager {
    MalkyEventBus  *bus;
    MalkyDialog    *dialog;
    MalkyConfig    *config;
    GPtrArray      *plugins;
    guint           dnd_handler;
};

static MalkyPluginManager *global_mgr = NULL;

/* ---- API wrappers ---- */

static void
api_dialog_say(const char *text, int timeout_ms)
{
    malky_dialog_say(global_mgr->dialog, text, timeout_ms);
}

static void
api_dialog_ask(const char *text,
               const char *btn1, const char *btn2,
               MalkyPluginChoiceCb callback,
               gpointer user_data,
               GDestroyNotify user_data_free)
{
    malky_dialog_ask(global_mgr->dialog, text,
                     btn1, btn2,
                     (MalkyDialogCb)callback,
                     user_data, user_data_free);
}

static char *
api_config_get(const char *key, const char *def)
{
    return malky_config_get_string(global_mgr->config, key, def);
}

typedef struct {
    MalkyPluginTaskCb  cb;
    gpointer           user_data;
} TaskContext;

static void
close_pid_only(GPid pid, int status, gpointer user_data)
{
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
}

static void
on_task_done(GPid pid, int status, gpointer user_data)
{
    TaskContext *tc = (TaskContext *)user_data;
    if (tc->cb)
        tc->cb(WEXITSTATUS(status), tc->user_data);
    g_spawn_close_pid(pid);
    g_free(tc);
}

static void
api_task_spawn(const char *const *argv,
               MalkyPluginTaskCb on_done,
               gpointer user_data)
{
    GPid child_pid;
    GError *error = NULL;

    if (!g_spawn_async(NULL, (gchar **)argv, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                       NULL, NULL, &child_pid, &error))
    {
        if (on_done)
            on_done(-1, user_data);
        g_error_free(error);
        return;
    }

    if (on_done)
    {
        TaskContext *tc = g_new(TaskContext, 1);
        tc->cb = on_done;
        tc->user_data = user_data;
        g_child_watch_add(child_pid, on_task_done, tc);
    }
    else
    {
        g_child_watch_add(child_pid, close_pid_only, NULL);
    }
}

static void
api_ensure_deps(const char *const *binary_names,
                const char *const *package_names,
                MalkyPluginInstallCb callback,
                gpointer user_data,
                GDestroyNotify user_data_free)
{
    malky_installer_ensure(binary_names, package_names,
                           global_mgr->dialog,
                           callback, user_data, user_data_free);
}

static const MalkyPluginAPI plugin_api = {
    .dialog_say  = api_dialog_say,
    .dialog_ask  = api_dialog_ask,
    .config_get  = api_config_get,
    .task_spawn  = api_task_spawn,
    .ensure_deps = api_ensure_deps,
};

/* ---- Plugin loading ---- */

static void
plugin_entry_free(gpointer data)
{
    PluginEntry *entry = (PluginEntry *)data;
    if (entry->info && entry->info->shutdown)
        entry->info->shutdown();
    if (entry->handle)
        g_module_close(entry->handle);
    g_free(entry->path);
    g_free(entry);
}

static void
try_load_plugin(MalkyPluginManager *mgr, const char *path)
{
    GModule *module = g_module_open(path, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
    if (!module)
        return;

    MalkyPluginInfo *(*get_info)(void) = NULL;
    if (!g_module_symbol(module, "malky_plugin_get_info",
                         (gpointer *)&get_info) || !get_info)
    {
        g_module_close(module);
        return;
    }

    MalkyPluginInfo *info = get_info();
    if (!info || !info->name || !info->init)
    {
        g_module_close(module);
        return;
    }

    if (!info->init(&plugin_api))
    {
        g_module_close(module);
        return;
    }

    PluginEntry *entry = g_new0(PluginEntry, 1);
    entry->handle = module;
    entry->info   = info;
    entry->path   = g_strdup(path);
    g_ptr_array_add(mgr->plugins, entry);

    MalkyEvent ev = {
        .type = M_EVT_PLUGIN_LOADED,
        .data = (gpointer)info->name,
    };
    malky_event_publish(mgr->bus, &ev);
}

static void
scan_directory(MalkyPluginManager *mgr, const char *dir_path)
{
    GDir *dir = g_dir_open(dir_path, 0, NULL);
    if (!dir) return;

    const char *name;
    while ((name = g_dir_read_name(dir)) != NULL)
    {
        gchar *full = g_build_filename(dir_path, name, NULL);

        if (g_file_test(full, G_FILE_TEST_IS_DIR))
            scan_directory(mgr, full);
        else if (g_str_has_suffix(name, ".so"))
            try_load_plugin(mgr, full);

        g_free(full);
    }

    g_dir_close(dir);
}

/* ---- DND dispatch ---- */

static gboolean
on_dnd_event(MalkyEvent *ev, gpointer user_data)
{
    MalkyPluginManager *mgr = (MalkyPluginManager *)user_data;
    if (ev->type != M_EVT_DND_RECEIVED || !ev->data)
        return FALSE;

    MalkyDndData *data = (MalkyDndData *)ev->data;

    MalkyPluginDndData pd;
    pd.type = (int)data->type;
    pd.uri  = data->uri;
    pd.path = data->path;

    for (guint i = 0; i < mgr->plugins->len; i++)
    {
        PluginEntry *entry = g_ptr_array_index(mgr->plugins, i);
        if (!entry->info->handle_dnd || !entry->info->dnd_types)
            continue;

        for (const int *t = entry->info->dnd_types; *t; t++)
        {
            if (*t == (int)data->type)
            {
                entry->info->handle_dnd(&pd);
                break;
            }
        }
    }

    return FALSE;
}

/* ---- Public API ---- */

MalkyPluginManager *
malky_plugin_manager_new(MalkyEventBus *bus,
                         MalkyDialog   *dialog,
                         MalkyConfig   *config)
{
    MalkyPluginManager *mgr = g_new0(MalkyPluginManager, 1);

    mgr->bus     = bus;
    mgr->dialog  = dialog;
    mgr->config  = config;
    mgr->plugins = g_ptr_array_new_with_free_func(plugin_entry_free);

    global_mgr = mgr;

    mgr->dnd_handler = malky_event_subscribe(mgr->bus, M_EVT_DND_RECEIVED,
                                             on_dnd_event, mgr, NULL);

    return mgr;
}

void
malky_plugin_manager_free(MalkyPluginManager *mgr)
{
    if (!mgr) return;

    malky_event_unsubscribe(mgr->bus, mgr->dnd_handler);

    if (mgr->plugins)
        g_ptr_array_unref(mgr->plugins);

    if (global_mgr == mgr)
        global_mgr = NULL;

    g_free(mgr);
}

void
malky_plugin_manager_scan(MalkyPluginManager *mgr, const char *directory)
{
    scan_directory(mgr, directory);
}

int
malky_plugin_manager_count(MalkyPluginManager *mgr)
{
    return (int)mgr->plugins->len;
}
