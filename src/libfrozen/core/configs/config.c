#include <libfrozen.h>

char  defconfig_m4_path[] = M4PATH;
char  defconfig_m4_incs[] = "";
char  defconfig_m4_opts[] = "";

char *config_m4_path      = defconfig_m4_path;
char *config_m4_incs      = defconfig_m4_incs;
char *config_m4_opts      = defconfig_m4_opts;

typedef struct filetype {
	config_format          format;
	config_wrapper         wrapper;
} filetype;

static filetype configs_get_filetype(char *filename){ // {{{
	char                  *ext;
	char                  *ext_old           = NULL;
	filetype               filetype          = { CONFIG_PLAIN, 0 };
	
	if( (ext = strrchr(filename, '.')) == NULL)
		goto exit;
	
	if(strcasecmp(ext, ".m4") == 0){
		filetype.wrapper = CONFIG_M4;
		
		 ext_old = ext;
		*ext_old = '\0';
		
		if( (ext = strrchr(filename, '.')) == NULL)
			goto exit;
	}
	
	     if(strcasecmp(ext, ".fz")   == 0){ filetype.format = CONFIG_DATAORIENTED;    }
	else if(strcasecmp(ext, ".conf") == 0){ filetype.format = CONFIG_PLAIN;           }
	else                                  { filetype.format = CONFIG_PLAIN;           }
	
exit:
	if(ext_old)
		*ext_old = '.';
	
	return filetype;
} // }}}
static char *   configs_fd_to_string(FILE *fd){ // {{{
	char                  *content           = NULL;
	uintmax_t              content_off       = 0;
	
	while(!feof(fd)){
		content       = realloc(content, content_off + DEF_BUFFER_SIZE + 1); // 1 for terminating \0
		if(!content)
			break;
		
		content_off  += fread(content + content_off, 1, DEF_BUFFER_SIZE, fd);
	}
	content[content_off] = '\0';
	return content;
} // }}}
static char *   configs_read_file(char *filename, filetype filetype){ // {{{
	FILE                  *fd;
	char                   buffer[DEF_BUFFER_SIZE];
	char                  *content           = NULL;
	
	switch(filetype.wrapper){
		case CONFIG_M4:
			if(snprintf(
				buffer, sizeof(buffer),
				"%s %s -s %s %s",
				config_m4_path,
				config_m4_incs,
				config_m4_opts,
				filename
			) > sizeof(buffer)) // -s for sync lines
				return NULL;
			
			if( (fd = popen(buffer, "r")) == NULL)
				return NULL;
			
			if( (content = configs_fd_to_string(fd)) == NULL)
				return NULL;
			
			pclose(fd);
			break;
			
		default:
			if( (fd = fopen(filename, "rb")) == NULL)
				return NULL;
			
			if( (content = configs_fd_to_string(fd)) == NULL)
				return NULL;
			
			fclose(fd);
			break;
	}
	return content;
} // }}}

hash_t *   configs_string_parse (char *string, config_format format){ // {{{
	switch(format){
		case CONFIG_PLAIN:        return configs_pl_string_parse(string);
		case CONFIG_DATAORIENTED: return configs_fz_string_parse(string);
		default:
			break;
	}
	return NULL;
} // }}}
hash_t *   configs_file_parse   (char *filename){ // {{{
	filetype               filetype;
	char                  *content           = NULL;
	hash_t                *hash              = NULL;
	
	filetype = configs_get_filetype(filename);
	
	if( (content = configs_read_file(filename, filetype)) == NULL)
		return NULL;
	
	config_ext_file = strdup(filename);
	
	hash = configs_string_parse(content, filetype.format);
	
	free(content);
	
	if(config_ext_file){
		free(config_ext_file);
		config_ext_file = NULL;
	}
	return hash;
} // }}}
