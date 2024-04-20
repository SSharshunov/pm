#include "log.h"
#include "version.h"
#include <pm.h>
#include <assert.h>
#include <plugin.h>

static void on_sigint_cb(evutil_socket_t sig, short events, void *ctx) {
    (void)sig;
    (void)events;

    struct event_base *base = ctx;

    printf("\n");
    log_warn("Caught an interrupt signal; exiting cleanly now.");

    struct timeval delay = {0, 200};
    event_base_loopexit(base, &delay);
}

int main(int argc, const char **argv) {
    log_set_level(LOG_LEVEL);

#ifndef SKIP_VERSION
    log_info("%s %s Version: %s, built on %s",
        get_project_name(), BUILD_TYPE, get_git_version(), get_builddate()
    );
#endif

    plugins_config_t *pc = load_config("pm.yaml");
    if (!pc) {
        log_error("Creating plugin manager failed");
        return EXIT_SUCCESS;
    }

    const char* plugin_path = find_config_node(pc->root, "plugin_dir");

    pm_t *pm = create_pm(pc, plugin_path, on_sigint_cb);
    if (!pm) {
        log_error("Creating plugin manager failed");
        goto exiting;
    }

    if (pm->plugins_path) log_trace("pm->plugins_path '%s'", pm->plugins_path);

    int count = list_count(pm->plugins);
    log_info("list_count(list): %d", count);
    if (count == 0) goto after_dispatch;

    event_base_dispatch(pm->base);

after_dispatch:
    free_pm(pm);
    pm = NULL;
exiting:

    ht_destroy(pc->root);
    destroy_plugin_config(pc->plugins);
    free(pc);
    pc = NULL;

    log_trace("Exit");
    return EXIT_SUCCESS;
}
