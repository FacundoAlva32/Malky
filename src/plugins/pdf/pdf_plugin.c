#include "plugin_api.h"

#include <string.h>
#include <stdio.h>

static const int dnd_types[] = { M_PLUGIN_DND_PDF, 0 };

static const MalkyPluginAPI *api;
static char *download_dir = NULL;

static char *pending_path = NULL;
static char *pending_merge_path = NULL;

static const char *
get_download_dir(void)
{
    if (download_dir)
        return download_dir;

    char *cfg = api->config_get("download_dir", NULL);
    if (cfg && cfg[0])
        download_dir = cfg;
    else
    {
        g_free(cfg);
        download_dir = g_strdup(
            g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
        if (!download_dir)
            download_dir = g_strdup(g_get_home_dir());
    }
    return download_dir;
}

static char *
strip_ext(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (dot && g_ascii_strcasecmp(dot, ".pdf") == 0)
        return g_strndup(path, dot - path);
    return g_strdup(path);
}

/* ── Done callbacks ─────────────────────────────────────── */

static void
on_operation_done(int exit_status, gpointer user_data)
{
    const char *label = (const char *)user_data;
    if (exit_status == 0)
    {
        char *msg = g_strdup_printf(
            "%s completado! Archivos en:\n%s",
            label, get_download_dir());
        api->dialog_say(msg, 5000);
        g_free(msg);
    }
    else
        api->dialog_say("Error al procesar el PDF. Revisa la terminal.", 5000);
}

/* ── Operations ─────────────────────────────────────────── */

static void
do_split(void)
{
    gchar *base = strip_ext(pending_path);
    gchar *name = g_path_get_basename(base);
    gchar *out_tmpl = g_strdup_printf("%s/%s_pagina_%%d.pdf",
                                      get_download_dir(), name);
    g_free(name);
    g_free(base);

    const char *argv[] = {
        "qpdf", "--split-pages", pending_path, out_tmpl, NULL,
    };

    api->dialog_say("Dividiendo PDF en paginas...", 60000);
    api->task_spawn(argv, on_operation_done, (gpointer)"Division");
    g_free(out_tmpl);
}

static void
do_repair(void)
{
    gchar *base = strip_ext(pending_path);
    gchar *name = g_path_get_basename(base);
    gchar *out = g_strdup_printf("%s/%s-reparado.pdf",
                                 get_download_dir(), name);
    g_free(name);
    g_free(base);

    const char *argv[] = {
        "qpdf", "--linearize", pending_path, out, NULL,
    };

    api->dialog_say("Reparando PDF...", 60000);
    api->task_spawn(argv, on_operation_done, (gpointer)"Reparacion");
    g_free(out);
}

static void
do_compress(void)
{
    gchar *base = strip_ext(pending_path);
    gchar *name = g_path_get_basename(base);
    gchar *out = g_strdup_printf("%s/%s-comprimido.pdf",
                                 get_download_dir(), name);
    g_free(name);
    g_free(base);

    const char *argv[] = {
        "qpdf", "--compress-streams=y", "--object-streams=generate",
        pending_path, out, NULL,
    };

    api->dialog_say("Comprimiendo PDF...", 60000);
    api->task_spawn(argv, on_operation_done, (gpointer)"Compresion");
    g_free(out);
}

static void
do_merge(const char *first, const char *second)
{
    gchar *base1 = strip_ext(first);
    gchar *base2 = strip_ext(second);
    gchar *n1 = g_path_get_basename(base1);
    gchar *n2 = g_path_get_basename(base2);
    gchar *out = g_strdup_printf("%s/%s+%s-unido.pdf",
                                 get_download_dir(), n1, n2);
    g_free(n2);
    g_free(n1);
    g_free(base2);
    g_free(base1);

    const char *argv[] = {
        "qpdf", "--empty", "--pages", first, second, "--", out, NULL,
    };

    api->dialog_say("Uniendo PDFs...", 60000);
    api->task_spawn(argv, on_operation_done, (gpointer)"Union");
    g_free(out);
}

/* ── Dialog callbacks ───────────────────────────────────── */

static void
third_choice(int choice, gpointer data)
{
    (void)data;
    if (choice < 0) {
        g_clear_pointer(&pending_merge_path, g_free);
        return;
    }

    if (choice == 0)
        do_compress();
    else
    {
        pending_merge_path = g_strdup(pending_path);
        g_clear_pointer(&pending_path, g_free);
        api->dialog_say("Solta otro PDF para unir.", 8000);
    }
}

static void
second_choice(int choice, gpointer data)
{
    (void)data;
    if (choice < 0) return;

    if (choice == 0)
        do_repair();
    else
        api->dialog_ask("Comprimir o Unir:",
            "Comprimir", "Unir",
            third_choice, NULL, NULL);
}

static void
first_choice(int choice, gpointer data)
{
    (void)data;
    if (choice < 0) return;

    if (choice == 0)
        do_split();
    else
        api->dialog_ask("Reparar o mas opciones:",
            "Reparar", "Mas opciones",
            second_choice, NULL, NULL);
}

/* ── Deps ready ─────────────────────────────────────────── */

static void
on_deps_ready(gboolean success, gpointer user_data)
{
    (void)user_data;
    if (!success)
    {
        g_clear_pointer(&pending_path, g_free);
        g_clear_pointer(&pending_merge_path, g_free);
        return;
    }

    if (pending_merge_path)
    {
        do_merge(pending_merge_path, pending_path);
        g_clear_pointer(&pending_merge_path, g_free);
        g_clear_pointer(&pending_path, g_free);
        return;
    }

    api->dialog_ask("PDF recibido! Que queres hacer?",
        "Dividir", "Reparar",
        first_choice, NULL, NULL);
}

/* ── Plugin entry points ────────────────────────────────── */

static bool
init(const MalkyPluginAPI *plugin_api)
{
    api = plugin_api;
    return true;
}

static void
shutdown(void)
{
    g_clear_pointer(&download_dir, g_free);
    g_clear_pointer(&pending_path, g_free);
    g_clear_pointer(&pending_merge_path, g_free);
    api = NULL;
}

static bool
handle_dnd(const MalkyPluginDndData *data)
{
    if (pending_merge_path)
    {
        // Second PDF arrived for merge — process immediately
        g_free(pending_path);
        pending_path = g_strdup(data->path ? data->path : data->uri);

        const char *bins[] = { "qpdf", NULL };
        const char *pkgs[] = { "qpdf", NULL };
        api->ensure_deps(bins, pkgs, on_deps_ready, NULL, NULL);
        return true;
    }

    g_free(pending_path);
    pending_path = g_strdup(data->path ? data->path : data->uri);

    api->dialog_say("Procesando PDF...", 1500);

    const char *bins[] = { "qpdf", NULL };
    const char *pkgs[] = { "qpdf", NULL };
    api->ensure_deps(bins, pkgs, on_deps_ready, NULL, NULL);

    return true;
}

MalkyPluginInfo malky_plugin_info = {
    .name        = "PDF Tool",
    .version     = "0.2.0",
    .description = "Divide, repara, comprime y une PDFs",
    .dnd_types   = dnd_types,
    .init        = init,
    .shutdown    = shutdown,
    .handle_dnd  = handle_dnd,
};

MalkyPluginInfo *
malky_plugin_get_info(void)
{
    return &malky_plugin_info;
}
