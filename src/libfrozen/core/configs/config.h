
extern char *config_m4_path;
extern char *config_m4_incs;
extern char *config_m4_opts;

API hash_t *   configs_string_parse (char *string);
API hash_t *   configs_file_parse   (char *filename);
