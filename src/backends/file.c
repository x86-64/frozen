#include <libfrozen.h>
#include <backend.h>

typedef struct file_user_data {
	char *           path;
	int              handle;
	pthread_mutex_t  create_lock;
	
} file_user_data;

static int file_init(chain_t *chain){
	file_user_data *user_data = malloc(sizeof(file_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	memset(user_data, 0, sizeof(file_user_data));
	
	chain->user_data = user_data;
	
	return 0;
}

static int file_destroy(chain_t *chain){
	file_user_data *data = (file_user_data *)chain->user_data;
	
	if(data->path){
		free(data->path);
		// was inited
		close(data->handle);
		pthread_mutex_destroy(&(data->create_lock));
	}
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	
	return 0;
}

static int file_configure(chain_t *chain, setting_t *config){
	char   *filepath;
	char   *filename;
	char   *temp;
	int     handle;
	
	filepath  = (char *)malloc(256);
	*filepath = '\0';
	
	filename = setting_get_child_string(config, "filename");
	if(filename == NULL){
		filename = setting_get_child_string(config, "backend_name");
		if(filename == NULL)
			goto cleanup;
	}
	
	temp = setting_get_child_string(global_settings, "homedir");
	if(temp == NULL)
		goto cleanup;
	
	
	if(snprintf(filepath, 256, "%s/%s.dat", temp, filename) >= 256){
		/* snprintf can truncate strings, so malicious user can use it to access another file.
		 * we check string len such way to ensure buffer not filled and none of strings was truncated */
		goto cleanup;
	}
	
	handle = open(
		filepath,
		O_CREAT | O_RDWR | O_LARGEFILE,
		S_IRUSR | S_IWUSR
	);
	
	if(handle == -1){
		goto cleanup;
	}
	
	file_user_data *data = (file_user_data *)chain->user_data;
	
	data->path   = filepath;
	data->handle = handle;
	
	pthread_mutex_init(&(data->create_lock), NULL);
	
	return 0;
cleanup:
	free(filepath);
	return -EINVAL;
}

static int file_create(chain_t *chain, void *key, size_t value_size){
	int               fd;
	int               ret;
	off_t             new_key;
	pthread_mutex_t  *mutex;
	

	fd    =  ((file_user_data *)chain->user_data)->handle;
	mutex = &((file_user_data *)chain->user_data)->create_lock;
	
	pthread_mutex_lock(mutex);
		
		new_key = lseek(fd, 0, SEEK_END);
		ret = ftruncate(fd, new_key + value_size);
		if(ret == -1){
			return -errno;
		}
		
		*(off_t *)key = new_key;
		
	pthread_mutex_unlock(mutex);
	
	return 0;
}

static int file_set(chain_t *chain, void *key, void *value, size_t value_size){
	int      fd;
	ssize_t  ret;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
	ret = pwrite(fd, value, value_size, *(off_t *)key);
	if(ret == -1 || ret != value_size){
		/* TODO error handling */
		fprintf(stderr, "TODO file.c\n");
		exit(255);
		return -EINVAL;
	}
	
	return 0;
}

static int file_get(chain_t *chain, void *key, void *value, size_t value_size){
	int      fd;
	ssize_t  ret;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
	ret = pread(fd, value, value_size, *(off_t *)key);
	if(ret == -1 || ret != value_size){
		/* TODO error handling */
		fprintf(stderr, "TODO file.c\n");
		exit(255);
		return -EINVAL;
	}
	
	return 0;
}

static int file_delete(chain_t *chain, void *key, size_t value_size){
	return 0;
}

static chain_t chain_file = {
	"file",
	CHAIN_TYPE_CRWD,
	&file_init,
	&file_configure,
	&file_destroy,
	{
		&file_create,
		&file_set,
		&file_get,
		&file_delete
	}
};
CHAIN_REGISTER(chain_file)

