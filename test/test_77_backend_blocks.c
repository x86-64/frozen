#include "test.h"

void read_all(buffer_t *buffer, backend_t *backend, off_t *key_off, unsigned int key_count, char *buf){
	ssize_t      ssize;
	unsigned int k;
	
	// check
	memset(buf, 0, 29);
	for(k=0; k<key_count; k++){
		hash_t  req_read[] = {
			{ TYPE_INT32, "action", (int []){ ACTION_CRWD_READ }, sizeof(int)   },
			{ TYPE_INT64, "key",    &key_off[k],                  sizeof(off_t) },
			{ TYPE_INT32, "size",   (int []){ 1                }, sizeof(int)   },
			hash_end
		};
		if( (ssize = backend_query(backend, req_read, buffer)) == 1){
			buffer_read(buffer, 0, &buf[k], 1);
		}
	}
}

START_TEST (test_backend_blocks){
	backend_t *backend;
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *settings = setting_new();
		setting_t *s_file = setting_new();
			setting_set_child_string(s_file, "name",        "file");
			setting_set_child_string(s_file, "filename",    "data_backend_blocks");
		setting_t *s_list = setting_new();
			setting_set_child_string(s_list, "name",        "blocks");
			setting_set_child_string(s_list, "block_size",  "4");
				setting_t *s_backend = setting_new_named("backend");
					setting_t *s_bfile = setting_new();
						setting_set_child_string(s_bfile, "name",        "file");
						setting_set_child_string(s_bfile, "filename",    "data_backend_blocks_map");
					setting_t *s_bloca = setting_new();
						setting_set_child_string(s_bloca, "name",        "blocks-address");
						setting_set_child_string(s_bloca, "per-level",   "4");
				setting_set_child_setting(s_backend, s_bfile);
				setting_set_child_setting(s_backend, s_bloca);
			setting_set_child_setting(s_list, s_backend);
	setting_set_child_setting(settings, s_file);
	setting_set_child_setting(settings, s_list);
	
	if( (backend = backend_new("in_block", settings)) == NULL){
		fail("backend creation failed");
		return;
	}
	
	
	ssize_t       ssize;
	buffer_t     *buffer = buffer_alloc();
	
	int           k;
	int           key_count = 28;
	off_t         key_off[key_count];
	off_t         key_from, key_to;
	char         *key_data  = malloc(100);
	char          chunk_1[] = "_`abcdefghijklmnopqrstuvwxyz";
	
	for(k=0; k < key_count; k++){
		hash_t  req_create[] = {
			{ TYPE_INT32, "action", (int []){ ACTION_CRWD_CREATE }, sizeof(int) },
			{ TYPE_INT32, "size",   (int []){ 1                  }, sizeof(int) },
			hash_end
		};
		
		if( (ssize = backend_query(backend, req_create, buffer)) <= 0 )
			fail("chain in_block create failed");	
		
		key_off[k] = 0;
		buffer_read(buffer, 0, &key_off[k], MIN(ssize, sizeof(off_t)));
			fail_unless(key_off[k] == k,                               "chain in_block create id failed");
	}
	for(k=0; k<key_count; k++){
		key_data[0] = (char)(0x5F + k);
		
		hash_t  req_write[] = {
			{ TYPE_INT32, "action", (int []){ ACTION_CRWD_WRITE  }, sizeof(int)   },
			{ TYPE_INT64, "key",    &key_off[k],                    sizeof(off_t) },
			{ TYPE_INT32, "size",   (int []){ 1                  }, sizeof(int)   },
			hash_end
		};
		
		buffer_write(buffer, 0, key_data, ssize);
		
		ssize = backend_query(backend, req_write, buffer);
			fail_unless(ssize == 1, "backend in_block write failed");
	}
	
	// check
	
	read_all(buffer, backend, (off_t *)&key_off, key_count, key_data);
		fail_unless(memcmp(key_data, chunk_1, key_count) == 0, "backend in_block read failed");
	//printf("was: %s\n", key_data);
	
	key_from = 1;
	key_to   = 2;
	hash_t  req_move[] = {
		{ TYPE_INT32, "action",   (int []){ ACTION_CRWD_MOVE   }, sizeof(int)   },
		{ TYPE_INT64, "key_from", &key_from,                      sizeof(off_t) },
		{ TYPE_INT64, "key_to",   &key_to,                        sizeof(off_t) },
		hash_end
	};
	ssize = backend_query(backend, req_move, buffer);
		fail_unless(ssize == 0, "backend in_block move failed");
	
	read_all(buffer, backend, (off_t *)&key_off, key_count, key_data);
	//	printf("from: %x to: %x res: %x, str: %s\n", (unsigned int)key_from, (unsigned int)key_to, (unsigned int)ssize, (char *)key_data);
	
	/*
	for(key_from = 0; key_from <= 8; key_from++){
		for(key_to = 0; key_to <= 8; key_to++){
			
			for(k=0; k<key_count; k++){
				key_data[0] = (char)(0x5F + k);
				
				hash_empty(hash);
				action  = ACTION_CRWD_WRITE;
				ssize   = 1;
					hash_set(hash, "action", TYPE_INT32,  &action);
					hash_set(hash, "size",   TYPE_INT32,  &ssize);
					hash_set(hash, "key",    TYPE_INT64,  &key_off[k]);
					
					buffer_write(buffer, 0, key_data, ssize);
				
				ssize = backend_query(backend, hash, buffer);
					fail_unless(ssize == 1, "backend in_block write failed");
			}
			
			hash_empty(hash);
			action   = ACTION_CRWD_MOVE;
			ssize    = 1;
				hash_set(hash, "action",   TYPE_INT32,  &action);
				hash_set(hash, "key_from", TYPE_INT64,  &key_from);
				hash_set(hash, "key_to",   TYPE_INT64,  &key_to);
			ssize = backend_query(backend, hash, buffer);
				fail_unless(ssize == 0, "backend in_block move failed");
			
			read_all(buffer, backend, &key_off, key_count, key_data);
			printf("from: %x to: %x res: %x, str: %s\n", (unsigned int)key_from, (unsigned int)key_to, (unsigned int)ssize, (char *)key_data);
		}
	}*/
	
	free(key_data);
	buffer_free(buffer);
	
	backend_destroy(backend);
	setting_destroy(settings);
}
END_TEST
REGISTER_TEST(core, test_backend_blocks)

