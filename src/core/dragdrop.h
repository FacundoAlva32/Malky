#pragma once

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _MalkyDragDrop MalkyDragDrop;
typedef struct _MalkyEventBus MalkyEventBus;

typedef enum {
    M_DND_UNKNOWN,
    M_DND_YOUTUBE_URL,
    M_DND_IMAGE_PNG,
    M_DND_IMAGE_JPEG,
    M_DND_IMAGE_WEBP,
    M_DND_IMAGE_AVIF,
    M_DND_IMAGE,
    M_DND_PDF,
    M_DND_VIDEO,
    M_DND_AUDIO,
    M_DND_ZIP,
    M_DND_TEXT,
} MalkyDndType;

typedef struct {
    MalkyDndType type;
    char        *uri;
    char        *path;
} MalkyDndData;

MalkyDragDrop *malky_dragdrop_new(MalkyEventBus *bus);
void           malky_dragdrop_free(MalkyDragDrop *dd);
void           malky_dragdrop_attach(MalkyDragDrop *dd, GtkWidget *widget);

const char    *malky_dnd_type_name(MalkyDndType type);
void           malky_dnd_data_free(MalkyDndData *data);
