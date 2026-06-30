#pragma once

#include <glib.h>

typedef struct _MalkyPluginManager MalkyPluginManager;
typedef struct _MalkyEventBus      MalkyEventBus;
typedef struct _MalkyDialog        MalkyDialog;
typedef struct _MalkyConfig        MalkyConfig;

MalkyPluginManager *malky_plugin_manager_new(MalkyEventBus *bus,
                                             MalkyDialog   *dialog,
                                             MalkyConfig   *config);
void                malky_plugin_manager_free(MalkyPluginManager *mgr);
void                malky_plugin_manager_scan(MalkyPluginManager *mgr,
                                             const char *directory);
int                 malky_plugin_manager_count(MalkyPluginManager *mgr);
