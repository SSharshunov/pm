#include <pm.h>

#include <unistd.h>
#include <log.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdlib.h>

void free_pm(pm_t *pm) {
    int (*plugin_destroy_cb)(plugin_t *);

    while (list_count(pm->plugins)) {
        plugin_t *plugin = list_pop_value(pm->plugins);

        plugin_destroy_cb = (int(*)(plugin_t *))dlsym(plugin->obj, "plugin_destroy");
        if (!plugin_destroy_cb) log_error("%s", dlerror());
        else ((int(*)(plugin_t *))plugin_destroy_cb)(plugin);

        dlclose(plugin->obj);
        free(plugin);
        plugin = NULL;
    }
    log_debug("list_count(list): %d", list_count(pm->plugins));
    list_destroy(pm->plugins);

    event_free(pm->sigint_handler);
    event_base_free(pm->base);

    free(pm->plugins_path);
    free(pm);
}

pm_t *create_pm(plugins_config_t *config, const char *plugins_path, void *on_sigint_cb) {
    if (!plugins_path) { log_warn("Plugin folder path is NULL"); return NULL; }

    log_info("Creating plugin manager, folder: '%s'", plugins_path);
    pm_t *pm = (pm_t*) calloc(1, sizeof(pm_t));
    if (!pm) { log_error("Error memory allocation"); return NULL; }
    pm->plugins_path = strdup(plugins_path);

    if ((pm->base = event_base_new()) == NULL) {
        log_error("event_base_new failed.");
        goto error;
    }

    if ((pm->sigint_handler = evsignal_new(pm->base, SIGINT, on_sigint_cb, (void *)pm->base)) == NULL) {
        log_error("evsignal_new failed.");
        goto error;
    }

    if (event_add(pm->sigint_handler, NULL) < 0) {
        log_error("event_add failed.");
        goto error;
    }

    pm->configs = config->plugins;

    pm->plugins = list_create();
    if (!pm->plugins) {
        log_error("Can't create a new list");
        goto error;
    }
    struct dirent *entry;

    log_trace("pm->plugins_path: '%s'", pm->plugins_path);
    DIR *dp = opendir(pm->plugins_path);
    if (!dp) {
        log_error("Cannot open directory %s", pm->plugins_path);
        goto error;
    }

    while ((entry = readdir(dp)) != NULL) {
        char *dot = strrchr(entry->d_name, '.');
        if (dot && !strcmp(dot, ".so")) plugin_register(pm, entry->d_name);
    }
    closedir(dp);
    return pm;
error:
    free(pm->plugins_path);
    free(pm);
    pm = NULL;
    return pm;
}

plugin_t *find_plugin_by_name(linked_list_t* plugins, char* name) {
    int count = list_count(plugins);

    for (int i = 0; i < count; i++) {
        plugin_t *plugin = (plugin_t *)list_pick_value(plugins, i);
        if (plugin) {
            if(strcmp(plugin->name, name) == 0)
                return plugin;
        }
    }
    return NULL;
}

char *get_config_value(hashtable_t *table, char* name) {
    config_node_t *item = (config_node_t *)ht_get(table, name, strlen(name), NULL);
    if (item) return (char*)item->value;
    else return NULL;
}

hashtable_t *find_config_by_name(linked_list_t* configs, const char* plugin_name) {
    int count = list_count(configs);

    for (int i = 0; i < count; i++) {
        hashtable_t *config_table = (hashtable_t *)list_pick_value(configs, i);
        if (config_table) {
            config_node_t *item = (config_node_t *)ht_get(config_table, "name", strlen("name"), NULL);
            // log_debug("item->value: '%s', plugin_name: '%s'", (char*)item->value, (char*)plugin_name);
            if (strcmp((char*)item->value, (char*)plugin_name) == 0)
                return config_table;
        }
    }
    return NULL;
}

int dump_ht_cb(hashtable_t *table, void *key, size_t klen, void *value, size_t vlen, void *user) {
    config_node_t *item = (config_node_t *)value;
    log_trace("'%s' %d '%s' %d", (char*)key, klen, (char*)item->value, item->vlen);
    return HT_ITERATOR_CONTINUE;
}

void dump_ht(hashtable_t *table) {
    int count = ht_count(table);
    // log_info("ht_count(ddata): %d", count);
    ht_foreach_pair(table, dump_ht_cb, NULL); // &check_item_count
    return;
}

char *events_names[] = {
    /** An empty event. */
    "YAML_NO_EVENT",
    /** A STREAM-START event. */
    "YAML_STREAM_START_EVENT",
    /** A STREAM-END event. */
    "YAML_STREAM_END_EVENT",
    /** A DOCUMENT-START event. */
    "YAML_DOCUMENT_START_EVENT",
    /** A DOCUMENT-END event. */
    "YAML_DOCUMENT_END_EVENT",
    /** An ALIAS event. */
    "YAML_ALIAS_EVENT",
    /** A SCALAR event. */
    "YAML_SCALAR_EVENT",
    /** A SEQUENCE-START event. */
    "YAML_SEQUENCE_START_EVENT",
    /** A SEQUENCE-END event. */
    "YAML_SEQUENCE_END_EVENT",
    /** A MAPPING-START event. */
    "YAML_MAPPING_START_EVENT",
    /** A MAPPING-END event. */
    "YAML_MAPPING_END_EVENT"
};

static inline void
ht_free_item_cb(config_node_t *node) {
    free(node->label);
    free(node->value);
    free(node);
}

static inline config_node_t *
create_node(char *label, void *value, size_t vlen) {
    if (!label) {
        log_error("Null label");
        return NULL;
    }
    config_node_t *node = calloc(1, sizeof(config_node_t));
    if (!node) {
        log_error("Cannot create config node");
        return NULL;
    }
    node->label = strdup(label);

    node->value = value;
    node->vlen = vlen;
    return node;
}

hashtable_t *parse_map(yaml_parser_t *parser);

linked_list_t *parse_seq(yaml_parser_t *parser) {
    yaml_event_t event;
    linked_list_t *llist = list_create();
    while (1) {
        if (!yaml_parser_parse(parser, &event)) {
            log_error("Parser error %d", parser->error);
            exit(EXIT_FAILURE);
        }
        if (event.type == YAML_SCALAR_EVENT) {
            log_info("\t\tYAML_SCALAR_EVENT %s", event.data.scalar.value);
        } else if (event.type == YAML_MAPPING_START_EVENT) {
            // log_info("\t%s", events_names[event.type]);
            hashtable_t *x = parse_map(parser);
            if (list_push_value(llist, x)) log_error("Can't push value");
        } else if (event.type == YAML_SEQUENCE_END_EVENT) {
            // log_info("%s", events_names[event.type]);
            yaml_event_delete(&event);
            break;
        } else {
            log_info("event.type == %d, %s", event.type, events_names[event.type]);
        }

        yaml_event_delete(&event);
    }
    return llist;
}

hashtable_t *parse_map(yaml_parser_t *parser) {
    yaml_event_t event;
    char *map_key = NULL;
    hashtable_t *table = ht_create(8, 1<<16, (ht_free_item_callback_t)ht_free_item_cb);
    if (!table) {
        log_error("Can't create a new hashtable");
    };
    while (1) {
        if (!yaml_parser_parse(parser, &event)) {
            log_error("Parser error %d", parser->error);
            exit(EXIT_FAILURE);
        }
        if (event.type == YAML_SCALAR_EVENT) {
            // log_info("\t\tYAML_SCALAR_EVENT %s", event.data.scalar.value);
            if (!map_key) {
                map_key = strdup((char*) event.data.scalar.value);
            } else {
                config_node_t *node = create_node(map_key, strdup((char*) event.data.scalar.value), strlen(event.data.scalar.value));
                if (ht_set(table, node->label, strlen(node->label), node, sizeof(config_node_t)) != 0) {
                    log_error("Cannot set to table");
                    return NULL;
                }
                free(map_key);
                map_key = NULL;
            }
        } else if (event.type == YAML_MAPPING_END_EVENT) {
            // log_info("\t%s", events_names[event.type]);
            yaml_event_delete(&event);
            break;
        } else {
            log_info("event.type == %d, %s", event.type, events_names[event.type]);
        }

        yaml_event_delete(&event);
    }
    // dump_ht(table);
    return table;
}

int destroy_iterator_cb(void *item, size_t idx, void *user) {
    hashtable_t *val = (hashtable_t *)item;
    ht_clear(val);
    ht_destroy(val);
    return 1;
}

void destroy_plugin_config(linked_list_t *llist) {
    list_foreach_value(llist, destroy_iterator_cb, (void *)NULL);
    list_destroy(llist);
    return;
}

linked_list_t *parse_plugins(yaml_parser_t *parser) {
    yaml_event_t event;
    char *map_key = NULL;
    int plugin_config_exists = 0;

    linked_list_t *llist = NULL;

    while (1) {
        if (!yaml_parser_parse(parser, &event)) {
            log_error("Parser error %d", parser->error);
            return NULL;
        }
        if (event.type == YAML_SEQUENCE_START_EVENT) {
            plugin_config_exists = 1;
            // log_info("%s", events_names[event.type]);
            llist = parse_seq(parser);
            break;
        } else if (event.type == YAML_SCALAR_EVENT) {
            log_info("\t\tYAML_SCALAR_EVENT '%s'", event.data.scalar.value);
            if (strcmp((char*)event.data.scalar.value, "") == 0) {
                yaml_event_delete(&event);
                if (map_key) free(map_key);
                map_key = NULL;
            }
        } else if (event.type == YAML_NO_EVENT) {
            log_info("YAML_NO_EVENT");
            yaml_event_delete(&event);
            break;
        } else {
            log_info("event.type == %d, %s", event.type, events_names[event.type]);
        }
        if (!plugin_config_exists) {
            log_warn("No config for plugins given");
            break;
        }
        yaml_event_delete(&event);
    }

    return llist;
}

hashtable_t *parse_root_map(yaml_parser_t *parser, plugins_config_t *conf) {
    yaml_event_t event;
    char *map_key = NULL;
    int not_implemented = 0;

    conf->root = ht_create(8, 1<<16, (ht_free_item_callback_t)ht_free_item_cb);
    if (!conf->root) {
        log_error("Can't create a new hashtable");
    };

    assert(conf->root);

    while (1) {
        if (!yaml_parser_parse(parser, &event)) {
            log_error("Parser error %d", parser->error);
            return NULL;
        }
        if (event.type == YAML_SEQUENCE_END_EVENT) {
            not_implemented = 0;
            log_info("%s", events_names[event.type]);
            free(map_key);
            map_key = NULL;
            continue;
        }
        if (not_implemented == 1) continue;
        if (event.type == YAML_SCALAR_EVENT) {
            // log_info("\t\tYAML_SCALAR_EVENT %s", event.data.scalar.value);
            if (strcmp((char*)event.data.scalar.value, "plugins") == 0) {
                conf->plugins = parse_plugins(parser);
            } else if (!map_key) {
                map_key = strdup((char*) event.data.scalar.value);
            } else {
                config_node_t *node = create_node(
                    map_key,
                    strdup((char*) event.data.scalar.value),
                    strlen(event.data.scalar.value)
                );

                if (ht_set(conf->root, node->label, strlen(node->label), node, sizeof(config_node_t)) != 0) {
                    log_error("Cannot set to table");
                    return NULL;
                }
                free(map_key);
                map_key = NULL;
            }
        } else if (event.type == YAML_SEQUENCE_START_EVENT) {
            not_implemented = 1;
            log_info("%s", events_names[event.type]);
        } else if (event.type == YAML_MAPPING_END_EVENT) {
            // log_trace("\t\tparse_map <<");
            // log_info("\t%s", events_names[event.type]);
            yaml_event_delete(&event);
            break;
        } else if (event.type == YAML_NO_EVENT) {
            log_info("YAML_NO_EVENT");
            yaml_event_delete(&event);
            break;
        } else {
            log_info("event.type == %d, %s", event.type, events_names[event.type]);
        }
        yaml_event_delete(&event);
    }
    // dump_ht(conf->root);
    return conf->root;
}

void get_from_yaml(yaml_parser_t *parser, plugins_config_t *conf) {
    yaml_event_t event; // = {0};

    while (1) {
        if (!yaml_parser_parse(parser, &event)) {
            log_error("Parser error %d", parser->error);
            exit(EXIT_FAILURE);
        }

        if (event.type == YAML_STREAM_START_EVENT ||
            event.type == YAML_DOCUMENT_START_EVENT ||
            event.type == YAML_DOCUMENT_END_EVENT)
        {
            // log_info("%s", events_names[event.type]);
        } else if (event.type == YAML_MAPPING_START_EVENT) {
            // log_info("\t%s", events_names[event.type]);
            // log_warn("parse_root_map");
            parse_root_map(parser, conf);
            return;
        } else if (event.type == YAML_SEQUENCE_START_EVENT) {
            // log_info("%s", events_names[event.type]);
            log_warn("Invalid YAML format");
            yaml_event_delete(&event);
            return;
        } else if (
            event.type == YAML_MAPPING_END_EVENT ||
            event.type == YAML_STREAM_END_EVENT ||
            event.type == YAML_NO_EVENT
        ) {
            // log_info("%s", events_names[event.type]);
            yaml_event_delete(&event);
            break;
            // return;
        } else {
            log_info("event.type == %d, %s", event.type, events_names[event.type]);
        }
        yaml_event_delete(&event);
    };
}

plugins_config_t *load_config(const char* filename) {

    if (access(filename, F_OK) != 0) {
        log_warn("Config file not found (%s)", filename);
        return NULL;
    }
#ifdef YAML
    yaml_parser_t parser;
    yaml_event_t event;

#ifdef NDEBUG
    yaml_parser_initialize(&parser);
#else
    assert(yaml_parser_initialize(&parser));
#endif

    log_info("Config filename: %s", filename);

    FILE *fp = fopen(filename, "rb");
    if (!fp) { log_error("Error opening file %s", filename); return NULL; }

    assert(fp);

// #define YAML_FROM_FILE 1

#ifdef YAML_FROM_FILE
    yaml_parser_set_input_file(&parser, fp);
#else
    fseek(fp, 0, SEEK_END); long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);  /* same as rewind(fp); */

    char *buffer = (char *) calloc(fsize + 1, sizeof(char));
    int len = fread(buffer, sizeof(char), fsize, fp);

    yaml_parser_set_input_string(
        &parser,
        buffer, // const unsigned char *input,
        len // size_t size
    );
#endif
    plugins_config_t *pc = (plugins_config_t*) calloc(1, sizeof(plugins_config_t));
    if (!pc) { log_error("Error memory allocation"); return NULL; }

    get_from_yaml(&parser, pc);

    if (!pc->root) log_error("pc->root is NULL");
    assert(pc->root);

    yaml_parser_delete(&parser);

#ifdef NDEBUG
    fclose(fp);
#else
    assert(!fclose(fp));
#endif

#ifndef YAML_FROM_FILE
    free(buffer);
    buffer = NULL;
#endif

#endif /* YAML */
    return pc;
}

char *find_config_node(hashtable_t *table, char* name) {
    assert(table);
    config_node_t *item = (config_node_t *)ht_get(table, name, strlen(name), NULL);
    if (item) return (char*)item->value;
    else return NULL;
}

int plugin_register(pm_t *pm, const char *filename) {
    size_t plugin_name_len = strlen(filename) - strlen("lib") - strlen(".so");
    char *plugin_name = calloc(plugin_name_len + 1, sizeof(char));
    memcpy(plugin_name, filename + strlen("lib"), plugin_name_len);
    hashtable_t *plugin_config = find_config_by_name(pm->configs, plugin_name);
    if (!plugin_config) {
        free(plugin_name);
        plugin_name = NULL;
        log_info("No config given for: %s", filename);
        return EXIT_FAILURE;
    }
    free(plugin_name);
    plugin_name = NULL;
    if (strcmp((char*)get_config_value(plugin_config, "disabled"), "true") == 0) {
        log_info("Plugin %s is disabled", filename);
        return EXIT_FAILURE;
    }

    log_debug("Loading plugin %s", filename);
    char path[255];
    sprintf(path, "%s/%s", pm->plugins_path, filename);
    void* obj = dlopen(path, RTLD_NOW);
    if (obj == NULL) {
        log_error("%s", dlerror());
        return EXIT_FAILURE;
    }

    init_cb f = (plugin_t *(*)(struct event_base *, hashtable_t *))dlsym(obj, "init");

    plugin_t *pl = NULL;
    if (!f) log_error("%s", dlerror());
    else {
        pl = (plugin_t*)f(pm->base, plugin_config);
        if (!pl) {
            log_error("plugin is NULL");
            return EXIT_FAILURE;
        }
#if 0
        log_trace("pl->name: '%s'", pl->name);
        log_trace("pl->instruction: %s", pl->instruction);
        ((void(*)())pl->func_ptr)();
#endif
        pl->obj = obj;
        if (list_push_value(pm->plugins, pl)) log_error("Can't push value");
    }
    return EXIT_SUCCESS;
}
