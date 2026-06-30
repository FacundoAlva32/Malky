#include "installer.h"
#include "dialog.h"

#include <string.h>
#include <sys/wait.h>

gboolean
malky_installer_check(const char *const *binary_names)
{
    for (const char *const *b = binary_names; *b; b++)
    {
        gchar *path = g_find_program_in_path(*b);
        if (!path) return FALSE;
        g_free(path);
    }
    return TRUE;
}

/* ---- state kept alive during async install ---- */

typedef struct {
    MalkyDialog    *dialog;
    MalkyInstallCb  callback;
    gpointer        user_data;
    GDestroyNotify  user_data_free;
    const char     *pm;
    gchar         **missing_bins;
    gchar         **pkg_names;
    int             missing_count;
} InstallState;

static const char *
detect_pm(void)
{
    if (g_find_program_in_path("dnf"))
        return "dnf";
    if (g_find_program_in_path("apt"))
        return "apt";
    if (g_find_program_in_path("pacman"))
        return "pacman";
    if (g_find_program_in_path("zypper"))
        return "zypper";
    return "dnf";
}

static void
build_pm_argv(const char *pm, const char *const *pkgs, GPtrArray *argv)
{
    g_ptr_array_add(argv, (gchar *)"pkexec");
    g_ptr_array_add(argv, (gchar *)pm);

    if (strcmp(pm, "dnf") == 0 || strcmp(pm, "zypper") == 0)
    {
        g_ptr_array_add(argv, (gchar *)"install");
        g_ptr_array_add(argv, (gchar *)"-y");
    }
    else if (strcmp(pm, "apt") == 0)
    {
        g_ptr_array_add(argv, (gchar *)"install");
        g_ptr_array_add(argv, (gchar *)"-y");
    }
    else if (strcmp(pm, "pacman") == 0)
    {
        g_ptr_array_add(argv, (gchar *)"-S");
        g_ptr_array_add(argv, (gchar *)"--noconfirm");
    }

    for (const char *const *p = pkgs; *p; p++)
        g_ptr_array_add(argv, (gchar *)*p);
}

static void
install_finished(GPid pid, int status, gpointer user_data)
{
    InstallState *s = (InstallState *)user_data;
    gboolean success = (WIFEXITED(status) && WEXITSTATUS(status) == 0);

    g_spawn_close_pid(pid);

    if (success)
    {
        malky_dialog_say(s->dialog,
            "Instalacion completada! Arrancando descarga...", 2000);
    }
    else
    {
        malky_dialog_say(s->dialog,
            "Error al instalar. Prob\u00e1 manualmente en la terminal:\n"
            "  sudo dnf install yt-dlp ffmpeg",
            6000);
    }

    MalkyInstallCb cb = s->callback;
    gpointer data = s->user_data;
    GDestroyNotify free = s->user_data_free;

    g_strfreev(s->missing_bins);
    g_strfreev(s->pkg_names);
    g_free(s);

    if (cb) cb(success, data);
    if (free) free(data);
}

static void
on_install_choice(int choice, gpointer user_data)
{
    InstallState *s = (InstallState *)user_data;

    if (choice != 0)
    {
        MalkyInstallCb cb = s->callback;
        gpointer data = s->user_data;
        GDestroyNotify free = s->user_data_free;
        g_strfreev(s->missing_bins);
        g_strfreev(s->pkg_names);
        g_free(s);
        if (cb) cb(FALSE, data);
        if (free) free(data);
        return;
    }

    const char *pm = s->pm;
    gchar *bin_list = g_strjoinv(", ", s->missing_bins);
    gchar *msg = g_strdup_printf(
        "Instalando %s... (puede pedir contrase\u00f1a)", bin_list);
    malky_dialog_say(s->dialog, msg, 120000);
    g_free(msg);
    g_free(bin_list);

    GPtrArray *argv = g_ptr_array_new();
    const char *const *pkgs = (const char *const *)s->pkg_names;
    build_pm_argv(pm, pkgs, argv);
    g_ptr_array_add(argv, NULL);

    GPid child_pid;
    GError *error = NULL;

    if (!g_spawn_async(NULL, (gchar **)argv->pdata, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                       NULL, NULL, &child_pid, &error))
    {
        malky_dialog_say(s->dialog,
            "Error al ejecutar pkexec. Prob\u00e1 en la terminal:\n"
            "  sudo dnf install yt-dlp ffmpeg",
            6000);
        MalkyInstallCb cb = s->callback;
        gpointer data = s->user_data;
        GDestroyNotify free = s->user_data_free;
        g_ptr_array_unref(argv);
        g_strfreev(s->missing_bins);
        g_strfreev(s->pkg_names);
        g_free(s);
        if (cb) cb(FALSE, data);
        if (free) free(data);
        g_error_free(error);
        return;
    }

    g_ptr_array_unref(argv);
    g_child_watch_add(child_pid, install_finished, s);
}

void
malky_installer_ensure(const char *const *binary_names,
                       const char *const *package_names,
                       MalkyDialog *dialog,
                       MalkyInstallCb callback,
                        gpointer user_data,
                        GDestroyNotify user_data_free)
{
    int missing_count = 0;
    for (const char *const *b = binary_names; *b; b++)
    {
        gchar *path = g_find_program_in_path(*b);
        if (!path) missing_count++;
        else g_free(path);
    }

    if (missing_count == 0)
    {
        if (callback) callback(TRUE, user_data);
        else if (user_data_free) user_data_free(user_data);
        return;
    }

    // Build missing names list for display
    GPtrArray *missing = g_ptr_array_new();
    for (const char *const *b = binary_names; *b; b++)
    {
        gchar *path = g_find_program_in_path(*b);
        if (!path) g_ptr_array_add(missing, g_strdup(*b));
        else g_free(path);
    }
    g_ptr_array_add(missing, NULL);

    // Copy package names
    int pkg_count = 0;
    for (const char *const *p = package_names; *p; p++) pkg_count++;
    gchar **pkg_copy = g_new(gchar *, pkg_count + 1);
    for (int i = 0; i < pkg_count; i++)
        pkg_copy[i] = g_strdup(package_names[i]);
    pkg_copy[pkg_count] = NULL;

    InstallState *s = g_new0(InstallState, 1);
    s->dialog = dialog;
    s->callback = callback;
    s->user_data = user_data;
    s->user_data_free = user_data_free;
    s->pm = detect_pm();
    s->missing_bins = (gchar **)g_ptr_array_free(missing, FALSE);
    s->pkg_names = pkg_copy;
    s->missing_count = missing_count;

    gchar *missing_str = g_strjoinv("\n  \u2022 ", s->missing_bins);
    gchar *ask_text = g_strdup_printf(
        "Para continuar necesito instalar:\n"
        "  \u2022 %s\n\n"
        "Quer\u00e9s que lo instale autom\u00e1ticamente?",
        missing_str);
    g_free(missing_str);

    malky_dialog_ask(dialog, ask_text,
                     "Instalar", "Cancelar",
                     on_install_choice, s, NULL);
    g_free(ask_text);
}
