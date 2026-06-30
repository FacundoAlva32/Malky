#pragma once

#include <glib.h>
#include <stdint.h>

typedef enum {
    M_EVT_NONE,
    M_EVT_QUIT,
    M_EVT_CONFIG_CHANGED,
    M_EVT_TIMER_TICK_1S,
    M_EVT_TIMER_TICK_10S,
    M_EVT_PET_CLICKED,
    M_EVT_DND_RECEIVED,
    M_EVT_PLUGIN_LOADED,
    M_EVT_PLUGIN_UNLOADED,
    M_EVT_PLUGIN_TASK_STARTED,
    M_EVT_PLUGIN_TASK_FINISHED,
    M_EVT_PLUGIN_TASK_FAILED,
    M_EVT_COUNT,
} MalkyEventType;

typedef struct {
    MalkyEventType type;
    gpointer       data;
    GDestroyNotify data_destroy;
    gboolean       handled;
} MalkyEvent;

typedef gboolean (*MalkyEventCb)(MalkyEvent *ev, gpointer user_data);

typedef struct _MalkyEventBus MalkyEventBus;

MalkyEventBus *malky_event_bus_new(void);
void           malky_event_bus_free(MalkyEventBus *bus);

guint malky_event_subscribe(MalkyEventBus *bus,
                            MalkyEventType type,
                            MalkyEventCb callback,
                            gpointer user_data,
                            GDestroyNotify user_data_free);

void malky_event_unsubscribe(MalkyEventBus *bus, guint handler_id);
void malky_event_publish(MalkyEventBus *bus, MalkyEvent *event);

const char *malky_event_type_name(MalkyEventType type);
