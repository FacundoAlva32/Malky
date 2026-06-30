#pragma once

#include <gtk/gtk.h>

typedef struct _MalkyApp      MalkyApp;
typedef struct _MalkyEventBus MalkyEventBus;
typedef struct _MalkyConfig   MalkyConfig;
typedef struct _MalkyWindow   MalkyWindow;
typedef struct _MalkyMovement MalkyMovement;
typedef struct _MalkyDialog   MalkyDialog;
typedef struct _MalkyPet      MalkyPet;
typedef struct _MalkyDragDrop MalkyDragDrop;

MalkyApp *malky_app_new(void);
int       malky_app_run(MalkyApp *app);
void      malky_app_free(MalkyApp *app);

MalkyEventBus *malky_app_get_event_bus(MalkyApp *app);
MalkyConfig   *malky_app_get_config(MalkyApp *app);
MalkyWindow   *malky_app_get_window(MalkyApp *app);
MalkyPet      *malky_app_get_pet(MalkyApp *app);
