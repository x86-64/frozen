#ifndef SETTINGS_H
#define SETTINGS_H

typedef enum setting_types {
	SETTING_NONE,
	SETTING_STRING,
	SETTING_LIST
} setting_types;

typedef struct setting_t {
	char *         name;
	setting_types  type;
	union {
		struct {
			char *   value;
		} string;
		struct {
			list     list; // list of setting_t
		} list;
	};
} setting_t;

API setting_t *  setting_new(void);
API setting_t *  setting_new_named(char *name);
API void         setting_destroy(setting_t *setting);
API void         setting_empty(setting_t *setting);

API void         setting_set_string(setting_t *setting, char *value);
API char *       setting_get_string(setting_t *setting);

API int          setting_iter_child(setting_t *setting, iter_callback func, void *arg1, void *arg2);
API void         setting_set_child_string(setting_t *setting, char *name, char *value);
API char *       setting_get_child_string(setting_t *setting, char *name);
API void         setting_set_child_setting(setting_t *setting, setting_t *child);
API setting_t *  setting_get_child_setting(setting_t *setting, char *name);
API void         setting_destroy_child(setting_t *setting, char *name);

#endif // SETTINGS_H
