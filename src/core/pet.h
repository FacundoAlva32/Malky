#pragma once

#include <glib.h>
#include <cairo.h>

typedef struct _MalkyPet      MalkyPet;
typedef struct _MalkyMovement MalkyMovement;
typedef struct _MalkyDialog   MalkyDialog;
typedef struct _MalkySkin     MalkySkin;

typedef enum {
    M_PET_IDLE,
    M_PET_WALKING,
    M_PET_SITTING,
    M_PET_SLEEPING,
    M_PET_HAPPY,
    M_PET_SAD,
    M_PET_SURPRISED,
    M_PET_COUNT,
} MalkyPetState;

MalkyPet *malky_pet_new(MalkyMovement *movement, MalkyDialog *dialog);
void      malky_pet_free(MalkyPet *pet);

void          malky_pet_update(MalkyPet *pet, double dt);
void          malky_pet_draw(MalkyPet *pet, cairo_t *cr,
                             int window_w, int window_h);
void          malky_pet_set_state(MalkyPet *pet, MalkyPetState state);
MalkyPetState malky_pet_get_state(MalkyPet *pet);
void          malky_pet_click(MalkyPet *pet);
void          malky_pet_say(MalkyPet *pet, const char *text, int timeout_ms);
void          malky_pet_handle_dnd(MalkyPet *pet, int dnd_type, const char *name);
gboolean      malky_pet_load_skin_dir(MalkyPet *pet, const char *dir);
MalkySkin    *malky_pet_get_skin(MalkyPet *pet);
const char   *malky_pet_state_name(MalkyPetState state);
