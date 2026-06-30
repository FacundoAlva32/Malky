#include "plugin_api.h"

#include <string.h>
#include <stdio.h>

static const int dnd_types[] = { M_PLUGIN_DND_YOUTUBE_URL, 0 };

static const MalkyPluginAPI *api;

static char *download_dir = NULL;

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

/* ---- download state ---- */

static int    pending_choice = 0;
static char  *pending_url    = NULL;

static void
on_download_done(int exit_status, gpointer user_data)
{
    (void)user_data;
    if (exit_status == 0)
    {
        char *msg = g_strdup_printf(
            "Descarga completada! Buscala en:\n%s",
            get_download_dir());
        api->dialog_say(msg, 5000);
        g_free(msg);
    }
    else
        api->dialog_say(
            "Hubo un error al descargar. yt-dlp tiro un error.",
            5000);
}

static void
start_download(void)
{
    gchar *out_tmpl = g_strdup_printf("%s/%%(title)s.%%(ext)s",
                                      get_download_dir());

    const char *argv[16];
    int i = 0;
    argv[i++] = "yt-dlp";
    argv[i++] = "--no-playlist";

    if (pending_choice == 0)
    {
        argv[i++] = "-x";
        argv[i++] = "--audio-format";
        argv[i++] = "mp3";
        argv[i++] = "--audio-quality";
        argv[i++] = "0";
    }
    else
    {
        argv[i++] = "-f";
        argv[i++] = "bestvideo+bestaudio";
        argv[i++] = "--merge-output-format";
        argv[i++] = "mp4";
    }

    argv[i++] = "-o";
    argv[i++] = out_tmpl;
    argv[i++] = pending_url;
    argv[i++] = NULL;

    const char *msg = (pending_choice == 0)
        ? "Descargando como MP3..."
        : "Descargando como MP4...";
    api->dialog_say(msg, 4000);
    api->task_spawn(argv, on_download_done, NULL);
    g_free(out_tmpl);
}

static void
on_deps_ready(gboolean success, gpointer user_data)
{
    (void)user_data;
    if (!success)
    {
        g_clear_pointer(&pending_url, g_free);
        pending_choice = 0;
        return;
    }
    start_download();
}

static void
on_format_chosen(int choice, gpointer user_data)
{
    pending_choice = choice;
    g_free(pending_url);
    pending_url = g_strdup((const char *)user_data);

    const char *bins[] = { "yt-dlp", "ffmpeg", NULL };
    const char *pkgs[] = { "yt-dlp", "ffmpeg", NULL };

    api->ensure_deps(bins, pkgs, on_deps_ready, NULL, NULL);
}

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
    g_clear_pointer(&pending_url, g_free);
    api = NULL;
}

static bool
handle_dnd(const MalkyPluginDndData *data)
{
    api->dialog_ask(
        "Quieres descargar el audio como MP3 o el video como MP4?",
        "MP3 (Audio)",
        "MP4 (Video)",
        on_format_chosen,
        g_strdup(data->uri),
        g_free);

    return true;
}

MalkyPluginInfo malky_plugin_info = {
    .name        = "YouTube Downloader",
    .version     = "0.1.0",
    .description = "Descarga audio/video de YouTube usando yt-dlp",
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
