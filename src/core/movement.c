#include "movement.h"

#include <math.h>

#define MARGIN 16
#define THRESHOLD 4.0

struct _MalkyMovement {
    double x, y;
    double target_x, target_y;
    double speed;
    double vx, vy;
    double bx, by, bw, bh;
    gboolean at_target;
};

MalkyMovement *
malky_movement_new(void)
{
    MalkyMovement *mov = g_new0(MalkyMovement, 1);
    mov->speed = 60.0;
    mov->bx = MARGIN;
    mov->by = MARGIN;
    mov->bw = 800 - MARGIN * 2;
    mov->bh = 600 - MARGIN * 2;
    mov->at_target = TRUE;
    return mov;
}

void
malky_movement_free(MalkyMovement *mov)
{
    g_free(mov);
}

void
malky_movement_set_position(MalkyMovement *mov, double x, double y)
{
    mov->x = x;
    mov->y = y;
}

void
malky_movement_get_position(MalkyMovement *mov, double *x, double *y)
{
    if (x) *x = mov->x;
    if (y) *y = mov->y;
}

void
malky_movement_set_target(MalkyMovement *mov, double x, double y)
{
    mov->target_x = CLAMP(x, mov->bx, mov->bx + mov->bw);
    mov->target_y = CLAMP(y, mov->by, mov->by + mov->bh);
    mov->at_target = FALSE;
}

void
malky_movement_get_target(MalkyMovement *mov, double *x, double *y)
{
    if (x) *x = mov->target_x;
    if (y) *y = mov->target_y;
}

void
malky_movement_set_speed(MalkyMovement *mov, double speed)
{
    mov->speed = speed;
}

void
malky_movement_set_bounds(MalkyMovement *mov, double x, double y, double w, double h)
{
    mov->bx = x;
    mov->by = y;
    mov->bw = w;
    mov->bh = h;
}

void
malky_movement_update(MalkyMovement *mov, double dt)
{
    if (mov->at_target) {
        mov->vx = 0;
        mov->vy = 0;
        return;
    }

    double dx = mov->target_x - mov->x;
    double dy = mov->target_y - mov->y;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist < THRESHOLD) {
        mov->x = mov->target_x;
        mov->y = mov->target_y;
        mov->vx = 0;
        mov->vy = 0;
        mov->at_target = TRUE;
        return;
    }

    double step = mov->speed * dt;
    if (step > dist) step = dist;

    double ratio = step / dist;
    mov->x += dx * ratio;
    mov->y += dy * ratio;

    mov->vx = dx / dist * mov->speed;
    mov->vy = dy / dist * mov->speed;

    mov->x = CLAMP(mov->x, mov->bx, mov->bx + mov->bw);
    mov->y = CLAMP(mov->y, mov->by, mov->by + mov->bh);
}

void
malky_movement_pick_random_target(MalkyMovement *mov)
{
    double x = mov->bx + g_random_double_range(0, mov->bw);
    double y = mov->by + g_random_double_range(0, mov->bh);
    malky_movement_set_target(mov, x, y);
}

gboolean
malky_movement_is_at_target(MalkyMovement *mov)
{
    return mov->at_target;
}


