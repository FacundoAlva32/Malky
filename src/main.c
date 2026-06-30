#include "core/app.h"

int
main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    MalkyApp *app = malky_app_new();
    if (!app) return 1;

    int ret = malky_app_run(app);
    malky_app_free(app);
    return ret;
}
