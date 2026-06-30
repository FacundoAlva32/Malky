#include "dragdrop.h"
#include "event.h"

#include <string.h>

struct _MalkyDragDrop {
    MalkyEventBus *event_bus;
};

static MalkyDndType
detect_type(const char *uri_or_path)
{
    if (strstr(uri_or_path, "youtube.com/watch") ||
        strstr(uri_or_path, "youtu.be/") ||
        strstr(uri_or_path, "youtube.com/shorts/"))
        return M_DND_YOUTUBE_URL;

    const char *ext = strrchr(uri_or_path, '.');
    if (!ext) return M_DND_UNKNOWN;
    ext++;

    if (g_ascii_strcasecmp(ext, "pdf") == 0)    return M_DND_PDF;
    if (g_ascii_strcasecmp(ext, "png") == 0)     return M_DND_IMAGE_PNG;
    if (g_ascii_strcasecmp(ext, "jpg") == 0 ||
        g_ascii_strcasecmp(ext, "jpeg") == 0)    return M_DND_IMAGE_JPEG;
    if (g_ascii_strcasecmp(ext, "webp") == 0)    return M_DND_IMAGE_WEBP;
    if (g_ascii_strcasecmp(ext, "avif") == 0)    return M_DND_IMAGE_AVIF;
    if (strcmp(ext, "svg") == 0)                 return M_DND_IMAGE;
    if (g_ascii_strcasecmp(ext, "mp4") == 0 ||
        g_ascii_strcasecmp(ext, "webm") == 0 ||
        g_ascii_strcasecmp(ext, "mkv") == 0)     return M_DND_VIDEO;
    if (g_ascii_strcasecmp(ext, "mp3") == 0 ||
        g_ascii_strcasecmp(ext, "flac") == 0 ||
        g_ascii_strcasecmp(ext, "wav") == 0 ||
        g_ascii_strcasecmp(ext, "ogg") == 0 ||
        g_ascii_strcasecmp(ext, "m4a") == 0)     return M_DND_AUDIO;
    if (g_ascii_strcasecmp(ext, "zip") == 0 ||
        g_ascii_strcasecmp(ext, "tar") == 0 ||
        g_ascii_strcasecmp(ext, "gz")  == 0 ||
        g_ascii_strcasecmp(ext, "rar") == 0 ||
        g_ascii_strcasecmp(ext, "7z")  == 0)     return M_DND_ZIP;

    return M_DND_UNKNOWN;
}

static void
publish_dnd_event(MalkyDragDrop *dd, MalkyDndType type,
                  const char *uri, const char *path)
{
    MalkyDndData *data = g_new0(MalkyDndData, 1);
    data->type = type;
    data->uri  = g_strdup(uri);
    data->path = g_strdup(path);

    MalkyEvent ev = {
        .type = M_EVT_DND_RECEIVED,
        .data = data,
        .data_destroy = (GDestroyNotify) malky_dnd_data_free,
    };
    malky_event_publish(dd->event_bus, &ev);
}

void
malky_dnd_data_free(MalkyDndData *data)
{
    if (!data) return;
    g_free(data->uri);
    g_free(data->path);
    g_free(data);
}

static gboolean
on_drop(GtkDropTarget *target, const GValue *value,
        double x, double y, gpointer user_data)
{
    (void)target;
    (void)x;
    (void)y;
    MalkyDragDrop *dd = (MalkyDragDrop *)user_data;

    char *uri = NULL;
    char *path = NULL;
    MalkyDndType type = M_DND_UNKNOWN;

    if (G_VALUE_HOLDS(value, G_TYPE_FILE)) {
        GFile *file = g_value_get_object(value);
        uri  = g_file_get_uri(file);
        path = g_file_get_path(file);
        if (path)
            type = detect_type(path);
    } else if (G_VALUE_HOLDS(value, G_TYPE_STRING)) {
        const char *text = g_value_get_string(value);

        if (g_str_has_prefix(text, "file://")) {
            uri  = g_strdup(text);
            path = g_filename_from_uri(text, NULL, NULL);
            if (path)
                type = detect_type(path);
        } else {
            uri  = g_strdup(text);
            path = NULL;
            type = detect_type(text);

            if (type == M_DND_UNKNOWN &&
                (g_str_has_prefix(text, "http://") ||
                 g_str_has_prefix(text, "https://"))) {
                if (strstr(text, "youtube.com") ||
                    strstr(text, "youtu.be"))
                    type = M_DND_YOUTUBE_URL;
                else
                    type = M_DND_TEXT;
            } else if (type == M_DND_UNKNOWN) {
                type = M_DND_TEXT;
            }
        }
    }

    if (uri)
        publish_dnd_event(dd, type, uri, path);
    else
        publish_dnd_event(dd, M_DND_TEXT, "", NULL);

    g_free(uri);
    g_free(path);
    return GDK_EVENT_STOP;
}

MalkyDragDrop *
malky_dragdrop_new(MalkyEventBus *bus)
{
    MalkyDragDrop *dd = g_new0(MalkyDragDrop, 1);
    dd->event_bus = bus;
    return dd;
}

void
malky_dragdrop_free(MalkyDragDrop *dd)
{
    g_free(dd);
}

void
malky_dragdrop_attach(MalkyDragDrop *dd, GtkWidget *widget)
{
    GtkDropTarget *target = gtk_drop_target_new(G_TYPE_INVALID,
                                                 GDK_ACTION_COPY);

    GType types[] = { G_TYPE_FILE, G_TYPE_STRING };
    gtk_drop_target_set_gtypes(target, types, G_N_ELEMENTS(types));

    g_signal_connect(target, "drop", G_CALLBACK(on_drop), dd);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(target));
}

const char *
malky_dnd_type_name(MalkyDndType type)
{
    switch (type) {
        case M_DND_UNKNOWN:      return "desconocido";
        case M_DND_YOUTUBE_URL:  return "YouTube";
        case M_DND_IMAGE_PNG:    return "PNG";
        case M_DND_IMAGE_JPEG:   return "JPEG";
        case M_DND_IMAGE_WEBP:   return "WEBP";
        case M_DND_IMAGE_AVIF:   return "AVIF";
        case M_DND_IMAGE:        return "imagen";
        case M_DND_PDF:          return "PDF";
        case M_DND_VIDEO:        return "video";
        case M_DND_AUDIO:        return "audio";
        case M_DND_ZIP:          return "archivo comprimido";
        case M_DND_TEXT:         return "texto";
    }
    return "?";
}
