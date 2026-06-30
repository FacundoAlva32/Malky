#pragma once

#include <glib.h>

typedef struct _MalkyMovement MalkyMovement;

MalkyMovement *malky_movement_new(void);
void           malky_movement_free(MalkyMovement *mov);

void     malky_movement_set_position(MalkyMovement *mov, double x, double y);
void     malky_movement_get_position(MalkyMovement *mov, double *x, double *y);
void     malky_movement_set_target(MalkyMovement *mov, double x, double y);
void     malky_movement_get_target(MalkyMovement *mov, double *x, double *y);
void     malky_movement_set_speed(MalkyMovement *mov, double speed);
void     malky_movement_set_bounds(MalkyMovement *mov, double x, double y, double w, double h);

void     malky_movement_update(MalkyMovement *mov, double dt);
void     malky_movement_pick_random_target(MalkyMovement *mov);
gboolean malky_movement_is_at_target(MalkyMovement *mov);

