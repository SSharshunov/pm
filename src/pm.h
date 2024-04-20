#ifndef PM_H
#define PM_H
#include <yaml.h>
#include <assert.h>
#include <plugin.h>


#include <signal.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>

typedef struct {
    char *plugins_path;
    linked_list_t *plugins;
    linked_list_t *configs;
    struct event *sigint_handler;
    struct event_base *base;
} pm_t;

char *find_config_node(hashtable_t *table, char* name);
plugins_config_t *load_config(const char* filename);
void destroy_plugin_config(linked_list_t *llist);

pm_t *create_pm(plugins_config_t *config, const char *plugins_path, void *on_sigint_cb);
void free_pm(pm_t *pm);
int plugin_register(pm_t *pm, const char *filename);
void plugin_execute(const char* path);

#endif /* PM_H */
