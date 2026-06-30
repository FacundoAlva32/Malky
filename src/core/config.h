#pragma once

#include <glib.h>

typedef struct _MalkyConfig MalkyConfig;
typedef struct _MalkyEventBus MalkyEventBus;

MalkyConfig *malky_config_new(MalkyEventBus *event_bus);
void         malky_config_free(MalkyConfig *config);

int     malky_config_get_int(MalkyConfig *config, const char *key, int def);
double  malky_config_get_double(MalkyConfig *config, const char *key, double def);
gboolean malky_config_get_bool(MalkyConfig *config, const char *key, gboolean def);
char   *malky_config_get_string(MalkyConfig *config, const char *key, const char *def);

void malky_config_set_int(MalkyConfig *config, const char *key, int val);
void malky_config_set_double(MalkyConfig *config, const char *key, double val);
void malky_config_set_bool(MalkyConfig *config, const char *key, gboolean val);
void malky_config_set_string(MalkyConfig *config, const char *key, const char *val);

gboolean malky_config_load(MalkyConfig *config);
gboolean malky_config_save(MalkyConfig *config);
