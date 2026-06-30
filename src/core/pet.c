#include "pet.h"
#include "skin.h"
#include "movement.h"
#include "dialog.h"
#include "dragdrop.h"

#define RADIUS 48

struct _MalkyPet {
    MalkyPetState  state;
    MalkyPetState  prev_state;
    MalkyMovement *movement;
    MalkyDialog   *dialog;
    MalkySkin     *skin;
    double         state_timer;
    double         boredom_timer;
    double         anim_frame;
    double         energy;
    double         motivation_timer;
    double         motivation_interval;
    double         long_phrase_timer;
    const char   **current_long_phrase;
    int            long_phrase_part;
    gboolean       long_phrase_active;
    int            long_phrase_count;
};

static const char *greetings[] = {
    "Gonorrea",
};

static const char *motivational[] = {
    "Un pajaro posado en un arbol nunca va a tener miedo de que la rama se rompa, porque la confianza no está en la rama sino en sus alas.",

};

static const char *long_phrases[] = {
    "No hay redención sin renuncia",
    "y no hay renuncia sin coraje",
    NULL,
    NULL,
};

static const char *startup_phrases[] = {
    "Hola! Encantada de conocerte!",
    "Lista para ayudarte!",
    "Jaja todo el dia rompiendo las bolas",
    "Arrastrame cosas y te ayudo!",
};

static const double STATE_TIMEOUTS[] = {
    [M_PET_IDLE]      = 0,
    [M_PET_WALKING]   = 0,
    [M_PET_SITTING]   = 8.0,
    [M_PET_SLEEPING]  = 60.0,
    [M_PET_HAPPY]     = 2.5,
    [M_PET_SAD]       = 3.0,
    [M_PET_SURPRISED] = 2.0,
};

// ── Long phrase helpers ─────────────────────────────────────

static int
count_long_phrases(void)
{
    int count = 0;
    const char **p = long_phrases;
    while (*p) {
        count++;
        while (*p) p++;
        p++;
    }
    return count;
}

static const char **
pick_long_phrase(int *part_out)
{
    int n = count_long_phrases();
    if (n == 0) return NULL;

    const char **p = long_phrases;
    int idx = g_random_int_range(0, n);
    for (int i = 0; i < idx; i++) {
        while (*p) p++;
        p++;
    }
    if (part_out) *part_out = 0;
    return p;
}

static void
reset_long_phrase(MalkyPet *pet)
{
    pet->long_phrase_active = FALSE;
    pet->current_long_phrase = NULL;
    pet->long_phrase_part = 0;
    pet->long_phrase_timer = 0;
}

// ── Pet implementation ───────────────────────────────────────

MalkyPet *
malky_pet_new(MalkyMovement *movement, MalkyDialog *dialog)
{
    MalkyPet *pet = g_new0(MalkyPet, 1);
    pet->movement = movement;
    pet->dialog = dialog;
    pet->state = M_PET_IDLE;
    pet->energy = 1.0;
    pet->anim_frame = g_random_double_range(0, 100);
    pet->motivation_interval = 45.0 + g_random_double_range(0, 45);
    pet->motivation_timer = 2.0;
    pet->long_phrase_active = FALSE;
    pet->current_long_phrase = NULL;
    pet->long_phrase_part = 0;
    pet->long_phrase_timer = 0;
    pet->long_phrase_count = count_long_phrases();
    pet->skin = malky_skin_new();

    // Try to load skin from standard locations
    gchar *user_skin = g_build_filename(g_get_user_data_dir(), "malky", "skins", "default", NULL);
    if (!malky_skin_load(pet->skin, user_skin))
    {
        gchar *exe = g_file_read_link("/proc/self/exe", NULL);
        if (exe)
        {
            gchar *exe_dir = g_path_get_dirname(exe);
            gchar *dev_skin = g_build_filename(exe_dir, "skins", "default", NULL);
            malky_skin_load(pet->skin, dev_skin);
            g_free(dev_skin);
            g_free(exe_dir);
            g_free(exe);
        }
    }
    g_free(user_skin);

    malky_dialog_say(pet->dialog,
        startup_phrases[g_random_int_range(0, G_N_ELEMENTS(startup_phrases))],
        3500);

    return pet;
}

void
malky_pet_free(MalkyPet *pet)
{
    if (!pet) return;
    g_clear_pointer(&pet->skin, malky_skin_free);
    g_free(pet);
}

void
malky_pet_set_state(MalkyPet *pet, MalkyPetState state)
{
    if (pet->state == state) return;

    pet->prev_state = pet->state;
    pet->state = state;
    pet->state_timer = 0;
    pet->boredom_timer = 0;
    if (state == M_PET_SLEEPING)
        reset_long_phrase(pet);
}

MalkyPetState
malky_pet_get_state(MalkyPet *pet)
{
    return pet->state;
}

MalkySkin *
malky_pet_get_skin(MalkyPet *pet)
{
    return pet->skin;
}

gboolean
malky_pet_load_skin_dir(MalkyPet *pet, const char *dir)
{
    return malky_skin_load(pet->skin, dir);
}

void
malky_pet_click(MalkyPet *pet)
{
    pet->motivation_timer = 0;

    if (pet->long_phrase_active && pet->current_long_phrase)
    {
        int part = pet->long_phrase_part + 1;
        const char *next = pet->current_long_phrase[part];

        if (!next)
        {
            reset_long_phrase(pet);
            if (pet->state != M_PET_SLEEPING && pet->state != M_PET_SURPRISED)
                malky_pet_set_state(pet, M_PET_HAPPY);
            return;
        }

        pet->long_phrase_part = part;
        pet->long_phrase_timer = 0;
        malky_dialog_say(pet->dialog, next, 0);
        return;
    }

    switch (pet->state) {
        case M_PET_SLEEPING:
            malky_dialog_say(pet->dialog,
                greetings[g_random_int_range(0, G_N_ELEMENTS(greetings))], 2500);
            malky_pet_set_state(pet, M_PET_SURPRISED);
            break;

        case M_PET_IDLE:
        case M_PET_SITTING:
        case M_PET_WALKING:
            malky_dialog_say(pet->dialog,
                greetings[g_random_int_range(0, G_N_ELEMENTS(greetings))], 2000);
            malky_pet_set_state(pet, M_PET_HAPPY);
            break;

        default:
            malky_pet_set_state(pet, M_PET_HAPPY);
            break;
    }
}

void
malky_pet_say(MalkyPet *pet, const char *text, int timeout_ms)
{
    if (!text || !text[0]) return;
    malky_dialog_say(pet->dialog, text, timeout_ms);
    if (pet->state != M_PET_SLEEPING && pet->state != M_PET_SURPRISED)
        malky_pet_set_state(pet, M_PET_HAPPY);
}

void
malky_pet_handle_dnd(MalkyPet *pet, int dnd_type, const char *name)
{
    (void)name;
    reset_long_phrase(pet);
    malky_pet_set_state(pet, M_PET_SURPRISED);

    switch (dnd_type) {
        case M_DND_YOUTUBE_URL:
            malky_dialog_say(pet->dialog,
                "Te ayudo con ese video de YouTube!",
                3000);
            break;
        case M_DND_PDF:
            malky_dialog_say(pet->dialog,
                "Un PDF! El plugin lo procesa.",
                2500);
            break;
        case M_DND_IMAGE_PNG:
        case M_DND_IMAGE_JPEG:
        case M_DND_IMAGE_WEBP:
        case M_DND_IMAGE_AVIF:
        case M_DND_IMAGE:
            malky_dialog_say(pet->dialog,
                "Una imagen! Quieres cambiarle el tamano o formato?",
                4000);
            break;
        case M_DND_VIDEO:
            malky_dialog_say(pet->dialog,
                "Un video! Quieres convertirlo a otro formato?",
                4000);
            break;
        case M_DND_AUDIO:
            malky_dialog_say(pet->dialog,
                "Un audio! Quieres convertirlo?",
                3500);
            break;
        case M_DND_ZIP:
            malky_dialog_say(pet->dialog,
                "Un comprimido! Quieres descomprimirlo?",
                3500);
            break;
        case M_DND_TEXT:
            malky_dialog_say(pet->dialog,
                "Has pegado un texto! Quieres que haga algo con esto?",
                4000);
            break;
        default:
            malky_dialog_say(pet->dialog,
                "Que es esto? No se que hacer con eso...",
                3000);
            break;
    }
}

void
malky_pet_update(MalkyPet *pet, double dt)
{
    pet->anim_frame += dt;
    pet->state_timer += dt;

    pet->motivation_timer += dt;
    if (pet->motivation_timer >= pet->motivation_interval
        && !malky_dialog_is_active(pet->dialog)
        && !pet->long_phrase_active
        && pet->state != M_PET_SLEEPING)
    {
        pet->motivation_timer = 0;
        pet->motivation_interval = 45.0 + g_random_double_range(0, 45);

        if (pet->long_phrase_count > 0
            && g_random_double_range(0, 1) < 0.5)
        {
            const char **phrase = pick_long_phrase(&pet->long_phrase_part);
            if (phrase) {
                pet->current_long_phrase = phrase;
                pet->long_phrase_active = TRUE;
                pet->long_phrase_timer = 0;
                malky_dialog_say(pet->dialog, phrase[0], 0);
            } else {
                malky_dialog_say(pet->dialog,
                    motivational[g_random_int_range(0, G_N_ELEMENTS(motivational))],
                    2500);
            }
        }
        else
        {
            malky_dialog_say(pet->dialog,
                motivational[g_random_int_range(0, G_N_ELEMENTS(motivational))],
                2500);
        }

        if (pet->state != M_PET_SLEEPING && pet->state != M_PET_SURPRISED)
            malky_pet_set_state(pet, M_PET_HAPPY);
    }

    switch (pet->state) {
        case M_PET_IDLE:
            pet->boredom_timer += dt;
            if (pet->state_timer > 30.0 && pet->energy < 0.3) {
                malky_dialog_say(pet->dialog, "Me voy a dormir...", 2000);
                malky_pet_set_state(pet, M_PET_SLEEPING);
            } else if (pet->boredom_timer > 5.0 + g_random_double_range(0, 5)) {
                malky_movement_pick_random_target(pet->movement);
                malky_pet_set_state(pet, M_PET_WALKING);
            }
            break;

        case M_PET_WALKING:
            malky_movement_update(pet->movement, dt);
            pet->energy -= dt * 0.005;
            pet->energy = MAX(pet->energy, 0.0);
            if (malky_movement_is_at_target(pet->movement)) {
                if (g_random_double_range(0, 1) < 0.25)
                    malky_pet_set_state(pet, M_PET_SITTING);
                else
                    malky_pet_set_state(pet, M_PET_IDLE);
            }
            break;

        case M_PET_SITTING:
            pet->energy += dt * 0.01;
            pet->energy = MIN(pet->energy, 1.0);
            if (pet->state_timer > STATE_TIMEOUTS[M_PET_SITTING]
                + g_random_double_range(0, 5))
            {
                malky_pet_set_state(pet, M_PET_IDLE);
            }
            break;

        case M_PET_SLEEPING:
            pet->energy += dt * 0.02;
            pet->energy = MIN(pet->energy, 1.0);
            if (pet->state_timer > STATE_TIMEOUTS[M_PET_SLEEPING]) {
                malky_dialog_say(pet->dialog, "Buenos dias!", 2000);
                malky_pet_set_state(pet, M_PET_HAPPY);
            }
            break;

        case M_PET_HAPPY:
            if (pet->state_timer > STATE_TIMEOUTS[M_PET_HAPPY]) {
                malky_pet_set_state(pet, M_PET_IDLE);
            }
            break;

        case M_PET_SAD:
            if (pet->state_timer > STATE_TIMEOUTS[M_PET_SAD]) {
                malky_pet_set_state(pet, M_PET_IDLE);
            }
            break;

        case M_PET_SURPRISED:
            if (pet->state_timer > STATE_TIMEOUTS[M_PET_SURPRISED]) {
                malky_pet_set_state(pet, M_PET_HAPPY);
            }
            break;

        default:
            break;
    }

    if (pet->long_phrase_active)
    {
        pet->long_phrase_timer += dt;
        if (pet->long_phrase_timer >= 300.0)
        {
            malky_dialog_clear(pet->dialog);
            reset_long_phrase(pet);
        }
    }

    if (pet->dialog)
        malky_dialog_update(pet->dialog, dt);
}

void
malky_pet_draw(MalkyPet *pet, cairo_t *cr, int window_w, int window_h)
{
    double cx = window_w / 2.0;
    double cy = window_h / 2.0;

    malky_skin_draw(pet->skin, cr, pet->state, pet->anim_frame, cx, cy);

    if (pet->dialog)
        malky_dialog_draw(pet->dialog, cr, cx, cy, RADIUS);
}

const char *
malky_pet_state_name(MalkyPetState state)
{
    switch (state) {
        case M_PET_IDLE:      return "IDLE";
        case M_PET_WALKING:   return "WALKING";
        case M_PET_SITTING:   return "SITTING";
        case M_PET_SLEEPING:  return "SLEEPING";
        case M_PET_HAPPY:     return "HAPPY";
        case M_PET_SAD:       return "SAD";
        case M_PET_SURPRISED: return "SURPRISED";
        default:              return "UNKNOWN";
    }
}
