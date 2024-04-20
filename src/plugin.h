#ifndef PLUGIN_H
#define PLUGIN_H

#include <linklist.h>
#include <hashtable.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>

#include <assert.h>

typedef enum {
    PLUGIN_INPUT_TYPE,
    PLUGIN_OUTPUT_TYPE
} plugin_type_t;

typedef struct {
    char *label;
    void *value;
    size_t vlen;
} config_node_t;

typedef struct {
    hashtable_t *root;
    linked_list_t *plugins;
} plugins_config_t;

typedef struct {
    char* name;
    void* obj;
    plugin_type_t type;
	char* instruction;
	void* func_ptr;
	int argc;
	struct event *events;
    void *ctx;
    void *user;
	linked_list_t *config;
} plugin_t;

typedef plugin_t * (*init_cb)(struct event_base *base, hashtable_t *plugin_config);

char *get_config_value(hashtable_t *table, char* name);

#endif /* PLUGIN_H */
