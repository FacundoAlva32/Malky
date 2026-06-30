#include "event.h"

typedef struct {
    MalkyEventType type;
    MalkyEventCb   callback;
    gpointer       user_data;
    GDestroyNotify user_data_free;
    guint          id;
    gboolean       active;
} Handler;

struct _MalkyEventBus {
    GPtrArray *handlers;  // of Handler*
    guint      next_id;
};

static void
handler_free(gpointer data)
{
    Handler *h = (Handler *)data;
    if (h->user_data_free && h->user_data)
        h->user_data_free(h->user_data);
    g_free(h);
}

MalkyEventBus *
malky_event_bus_new(void)
{
    MalkyEventBus *bus = g_new0(MalkyEventBus, 1);
    bus->handlers = g_ptr_array_new_with_free_func(handler_free);
    bus->next_id = 1;
    return bus;
}

void
malky_event_bus_free(MalkyEventBus *bus)
{
    if (!bus) return;
    g_ptr_array_unref(bus->handlers);
    g_free(bus);
}

guint
malky_event_subscribe(MalkyEventBus *bus,
                      MalkyEventType type,
                      MalkyEventCb callback,
                      gpointer user_data,
                      GDestroyNotify user_data_free)
{
    Handler *h = g_new0(Handler, 1);
    h->type     = type;
    h->callback = callback;
    h->user_data = user_data;
    h->user_data_free = user_data_free;
    h->id       = bus->next_id++;
    h->active   = TRUE;

    g_ptr_array_add(bus->handlers, h);
    return h->id;
}

void
malky_event_unsubscribe(MalkyEventBus *bus, guint handler_id)
{
    for (guint i = 0; i < bus->handlers->len; i++) {
        Handler *h = g_ptr_array_index(bus->handlers, i);
        if (h->id == handler_id) {
            h->active = FALSE;
            g_ptr_array_remove_index(bus->handlers, i);
            return;
        }
    }
}

void
malky_event_publish(MalkyEventBus *bus, MalkyEvent *event)
{
    event->handled = FALSE;

    for (guint i = 0; i < bus->handlers->len; i++) {
        Handler *h = g_ptr_array_index(bus->handlers, i);
        if (!h->active) continue;
        if (h->type != event->type && h->type != M_EVT_COUNT)
            continue;

        gboolean ret = h->callback(event, h->user_data);
        if (ret) {
            event->handled = TRUE;
            break;
        }
    }

    if (event->data_destroy && event->data)
        event->data_destroy(event->data);
}

const char *
malky_event_type_name(MalkyEventType type)
{
    switch (type) {
        case M_EVT_NONE:               return "NONE";
        case M_EVT_QUIT:               return "QUIT";
        case M_EVT_CONFIG_CHANGED:     return "CONFIG_CHANGED";
        case M_EVT_TIMER_TICK_1S:      return "TIMER_TICK_1S";
        case M_EVT_TIMER_TICK_10S:     return "TIMER_TICK_10S";
        case M_EVT_PET_CLICKED:        return "PET_CLICKED";
        case M_EVT_DND_RECEIVED:       return "DND_RECEIVED";
        case M_EVT_PLUGIN_LOADED:      return "PLUGIN_LOADED";
        case M_EVT_PLUGIN_UNLOADED:    return "PLUGIN_UNLOADED";
        case M_EVT_PLUGIN_TASK_STARTED: return "PLUGIN_TASK_STARTED";
        case M_EVT_PLUGIN_TASK_FINISHED: return "PLUGIN_TASK_FINISHED";
        case M_EVT_PLUGIN_TASK_FAILED:  return "PLUGIN_TASK_FAILED";
        case M_EVT_COUNT:              return "COUNT";
    }
    return "UNKNOWN";
}
