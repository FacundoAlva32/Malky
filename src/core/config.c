#include "config.h"
#include "event.h"

#include <stdio.h>
#include <string.h>

#define CONFIG_FILE "malky.conf"
#define MAX_LINE 1024

struct _MalkyConfig {
    MalkyEventBus *event_bus;
    GHashTable    *values;  // char* key -> char* value
    char          *path;
    gboolean       dirty;
};

MalkyConfig *
malky_config_new(MalkyEventBus *event_bus)
{
    MalkyConfig *config = g_new0(MalkyConfig, 1);
    config->event_bus = event_bus;
    config->values = g_hash_table_new_full(g_str_hash, g_str_equal,
                                            g_free, g_free);

    const char *xdg = g_get_user_config_dir();
    config->path = g_build_filename(xdg, "malky", CONFIG_FILE, NULL);

    g_mkdir_with_parents(g_path_get_dirname(config->path), 0755);
    malky_config_load(config);

    return config;
}

void
malky_config_free(MalkyConfig *config)
{
    if (!config) return;
    if (config->dirty) malky_config_save(config);
    g_hash_table_unref(config->values);
    g_free(config->path);
    g_free(config);
}

gboolean
malky_config_load(MalkyConfig *config)
{
    FILE *f = fopen(config->path, "r");
    if (!f) return FALSE;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        g_strstrip(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = g_strstrip(line);
        char *val = g_strstrip(eq + 1);

        if (key[0] && val[0])
            g_hash_table_insert(config->values, g_strdup(key), g_strdup(val));
    }

    fclose(f);
    return TRUE;
}

gboolean
malky_config_save(MalkyConfig *config)
{
    FILE *f = fopen(config->path, "w");
    if (!f) return FALSE;

    GHashTableIter iter;
    gpointer key, val;
    g_hash_table_iter_init(&iter, config->values);
    while (g_hash_table_iter_next(&iter, &key, &val))
        fprintf(f, "%s=%s\n", (const char *)key, (const char *)val);

    fclose(f);
    config->dirty = FALSE;
    return TRUE;
}

static const char *
config_get_raw(MalkyConfig *config, const char *key)
{
    return (const char *)g_hash_table_lookup(config->values, key);
}

int
malky_config_get_int(MalkyConfig *config, const char *key, int def)
{
    const char *val = config_get_raw(config, key);
    if (!val) return def;
    return (int)strtol(val, NULL, 10);
}

double
malky_config_get_double(MalkyConfig *config, const char *key, double def)
{
    const char *val = config_get_raw(config, key);
    if (!val) return def;
    return strtod(val, NULL);
}

gboolean
malky_config_get_bool(MalkyConfig *config, const char *key, gboolean def)
{
    const char *val = config_get_raw(config, key);
    if (!val) return def;
    return strcmp(val, "true") == 0 || strcmp(val, "1") == 0;
}

char *
malky_config_get_string(MalkyConfig *config, const char *key, const char *def)
{
    const char *val = config_get_raw(config, key);
    return g_strdup(val ? val : def);
}

void
malky_config_set_int(MalkyConfig *config, const char *key, int val)
{
    char buf[32];
    g_snprintf(buf, sizeof(buf), "%d", val);
    g_hash_table_insert(config->values, g_strdup(key), g_strdup(buf));
    config->dirty = TRUE;
}

void
malky_config_set_double(MalkyConfig *config, const char *key, double val)
{
    char buf[64];
    g_snprintf(buf, sizeof(buf), "%.6g", val);
    g_hash_table_insert(config->values, g_strdup(key), g_strdup(buf));
    config->dirty = TRUE;
}

void
malky_config_set_bool(MalkyConfig *config, const char *key, gboolean val)
{
    g_hash_table_insert(config->values, g_strdup(key),
                        g_strdup(val ? "true" : "false"));
    config->dirty = TRUE;
}

void
malky_config_set_string(MalkyConfig *config, const char *key, const char *val)
{
    g_hash_table_insert(config->values, g_strdup(key), g_strdup(val));
    config->dirty = TRUE;
}
