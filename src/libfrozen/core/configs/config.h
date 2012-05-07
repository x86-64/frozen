#ifndef CONFIGS_CONFIG_H
#define CONFIGS_CONFIG_H

typedef enum config_format {
	CONFIG_PLAIN          = 0x01,  // .conf extension
	CONFIG_DATAORIENTED   = 0x02,  // .fz   extension
} config_format;
#define config_format_mask   0x0f

typedef enum config_wrapper {
	CONFIG_M4             = 0x10, // .m4
} config_wrapper;
#define config_wrapper_mask  0xf0

extern char *config_m4_path;
extern char *config_m4_incs;
extern char *config_m4_opts;
extern char *config_ext_file;

API hash_t *   configs_pl_string_parse (char *string);
API hash_t *   configs_fz_string_parse (char *string);

API hash_t *   configs_string_parse (char *string, config_format format);
API hash_t *   configs_file_parse   (char *filename);

#endif
