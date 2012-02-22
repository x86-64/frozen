#include <libfrozen.h>
#include <dirent.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <numeric/uint/uint_t.h>
#include <core/string/string_t.h>

#define ERRORS_MODULE_ID 53
#define ERRORS_MODULE_NAME "storage/folder"

typedef struct folder_t {
	char                  *path;
} folder_t;

static void    folder_destroy(folder_t *fdata){ // {{{
	free(fdata->path);
	free(fdata);
} // }}}
static ssize_t folder_new(folder_t **pfdata, config_t *config){ // {{{
	ssize_t                ret;
	DIR                   *dir;
	folder_t              *fdata;
	char                   folderpath[DEF_BUFFER_SIZE];
	data_t                *cfg_folderpath;
	
	if( (fdata = calloc(1, sizeof(folder_t))) == NULL)
		return error("calloc returns null");
	
	if( (cfg_folderpath = hash_data_find(config, HK(path))) == NULL){
		ret = error("path not supplied");
		goto error;
	}

	fastcall_read r_read = { { 5, ACTION_READ }, 0, folderpath, sizeof(folderpath) - 1 };
	if(data_query(cfg_folderpath, &r_read) < 0){
		ret = error("path invalid");
		goto error;
	}
	folderpath[r_read.buffer_size] = '\0';
	
	if( (dir = opendir(folderpath)) == NULL){
		ret = error("path not exist");
		goto error;
	}
	closedir(dir);
	
	if( (fdata->path = strdup(folderpath)) == NULL){
		ret = -ENOMEM;
		goto error;
	}
	
	*pfdata = fdata;
	return 0;
	
error:
	free(fdata);
	return ret;
} // }}}

static ssize_t data_folder_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->src == NULL)
		return -EINVAL; 
		
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return folder_new((folder_t **)&data->ptr, config);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_folder_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr != NULL)
		folder_destroy(data->ptr);
	
	return 0;
} // }}}
static ssize_t data_folder_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	DIR                   *dir;
	struct dirent          dir_item_buf;
	struct dirent         *dir_item;
	char                   dir_item_path[DEF_BUFFER_SIZE * 2];
	folder_t              *fdata             = (folder_t *)data->ptr;
	
	if(
		fdata       == NULL                   ||
		fdata->path == NULL                   ||
		(dir = opendir(fdata->path)) == NULL
	)
		return -EINVAL;
	
	while(readdir_r(dir, &dir_item_buf, &dir_item) == 0 && dir_item != NULL){
		if(snprintf(
			dir_item_path, sizeof(dir_item_path),
			"%s/%s",
			fdata->path,
			dir_item->d_name
		) >= sizeof(dir_item_path))
			continue; // truncated path
		
		request_t r_next[] = {
			{ HK(name),     DATA_STRING(dir_item->d_name)  },
			{ HK(path),     DATA_STRING(dir_item_path)     },
			{ HK(inode),    DATA_UINTT(dir_item->d_ino)    },
		#ifdef _DIRENT_HAVE_D_TYPE
			{ HK(type),     DATA_UINTT(dir_item->d_type)   },
		#endif
		#ifdef _DIRENT_HAVE_D_RECLEN
			{ HK(reclen),   DATA_UINTT(dir_item->d_reclen) },
		#endif
		#ifdef _DIRENT_HAVE_D_OFF
			{ HK(offset),   DATA_UINTT(dir_item->d_off)    },
		#endif
			hash_end
		};
		data_t                 d_next            = DATA_PTR_HASHT(r_next);
		data_t                 d_copy;
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &d_copy };
		if( (ret = data_query(&d_next, &r_copy)) < 0)
			break;
		
		fastcall_push r_push = { { 3, ACTION_PUSH }, &d_copy };
		if( (ret = data_query(fargs->dest, &r_push)) < 0)
			break;
	}
	fastcall_push r_push = { { 3, ACTION_PUSH }, NULL };
	data_query(fargs->dest, &r_push);
	
	closedir(dir);
	return ret;
} // }}}

data_proto_t folder_t_proto = {
	.type                   = TYPE_FOLDERT,
	.type_str               = "folder_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_folder_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_folder_t_free,
		[ACTION_ENUM]         = (f_data_func)&data_folder_t_enum,
	}
};

