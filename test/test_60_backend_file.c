#include "test.h"

void  db_read(chain_t *chain, off_t key, size_t size, char *buf, ssize_t *ssize){
	buffer_t       *buffer = buffer_alloc();
	unsigned int    action;
	hash_t         *hash;
	ssize_t         ret;
	
	action = ACTION_CRWD_READ;
	hash   = hash_new();
		hash_set(hash, "action", TYPE_INT32, &action);
		hash_set(hash, "key",    TYPE_INT64, &key);
		hash_set(hash, "size",   TYPE_INT32, &size);
		
		ret = chain_query(chain, hash, buffer);
		if(ret > 0){
			buffer_read(buffer, 0, buf, ret);
		}
		*ssize = ret;
		
	hash_free(hash);
	buffer_free(buffer);
}

void  db_write(chain_t *chain, off_t key, char *buf, unsigned int buf_size, ssize_t *ssize){
	buffer_t        buffer;
	unsigned int    action;
	hash_t         *hash;
	
	buffer_init(&buffer);
	buffer_add_head_raw(&buffer, buf, buf_size);
	
	action  = ACTION_CRWD_WRITE;
	hash = hash_new();
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &key);
		hash_set(hash, "size",   TYPE_INT32,  &buf_size);
		
		*ssize = chain_query(chain, hash, &buffer);
	
	hash_free(hash);
	buffer_destroy(&buffer);
}

void  db_move(chain_t *chain, off_t key_from, off_t key_to, size_t key_size, ssize_t *ssize){
	buffer_t        buffer;
	unsigned int    action;
	hash_t         *hash;
	
	buffer_init(&buffer);
	
	action  = ACTION_CRWD_MOVE;
	hash = hash_new();
		hash_set(hash, "action",   TYPE_INT32, &action);
		hash_set(hash, "key_from", TYPE_INT64, &key_from); 
		hash_set(hash, "key_to",   TYPE_INT64, &key_to); 
		hash_set(hash, "size",     TYPE_INT32, &key_size);
		
		*ssize = chain_query(chain, hash, &buffer);
		
	hash_free(hash);
	buffer_destroy(&buffer);
}

START_TEST (test_backend_file){
	int             ret;
	ssize_t         ssize;
	off_t           new_key1, new_key2;
	buffer_t       *buffer  = buffer_alloc();
	hash_t         *hash;
	unsigned int    action;
	
	chain_t *chain = chain_new("file");
		fail_unless(chain != NULL, "chain 'file' not created");
	
	setting_t *setting = setting_new();
	setting_set_child_string(global_settings, "homedir",  ".");
	setting_set_child_string(setting,         "filename", "data_backend_file");
	
	ret = chain_configure(chain, setting);
		fail_unless(ret == 0, "chain 'file' configuration failed");
	
	action  = ACTION_CRWD_CREATE;
	hash = hash_new();
		hash_set(hash, "action", TYPE_INT32, &action);
		hash_set(hash, "size",   TYPE_INT32, "\x0a\x00\x00\x00");
		
		if( (ssize = chain_query(chain, hash, buffer)) != sizeof(off_t) )
			fail("chain file create key1 failed");	
		buffer_read(buffer, 0, &new_key1, MIN(ssize, sizeof(off_t)));
		
		if( (ssize = chain_query(chain, hash, buffer)) != sizeof(off_t) )
			fail("chain file create key2 failed");	
		buffer_read(buffer, 0, &new_key2, MIN(ssize, sizeof(off_t)));
			fail_unless(new_key2 - new_key1 == 10,                 "chain file offsets invalid");
	hash_free(hash);
	
	char  key1_data[]  = "test167890";
	char  key2_data[]  = "test267890";
	
	char  move1_data[] = "test161690";
	char  move2_data[] = "test161678";
	char  move3_data[] = "test787890";
	char  move4_data[] = "test789090";
	
	char *test_chunk = malloc(100);

	/* write {{{1 */
	db_write(chain, new_key1, key1_data, 10, &ssize);
		fail_unless(ssize == 10,                  "chain file key 1 set failed");
	
	db_write(chain, new_key2, key2_data, 10, &ssize);
		fail_unless(ssize == 10,                  "chain file key 2 set failed");
		
	/* }}}1 */
	/* read {{{1 */
	db_read(chain, new_key1, 10, test_chunk, &ssize); 
		fail_unless(ssize == 10,                               "chain file key 1 get failed");
		fail_unless(memcmp(test_chunk, key1_data, ssize) == 0, "chain file key 1 get data failed"); 
	
	db_read(chain, new_key2, 10, test_chunk, &ssize); 
		fail_unless(ssize == 10,                               "chain file key 2 get failed");
		fail_unless(memcmp(test_chunk, key2_data, ssize) == 0, "chain file key 2 get data failed"); 

	/* }}}1 */
	/* count {{{1 */
	size_t  count;
	
	action  = ACTION_CRWD_COUNT;
	hash = hash_new();
		hash_set(hash, "action", TYPE_INT32, &action);
		
		buffer_cleanup(buffer);
		
		ssize = chain_query(chain, hash, buffer);
			fail_unless(ssize > 0,                                "chain file count failed");
		
		buffer_read(buffer, 0, &count, MIN(ssize, sizeof(size_t)));
			fail_unless( (count / 20) >= 1,                       "chain file count failed");
		
	hash_free(hash);
	/* }}}1 */
	/* move {{{1 */
	db_write(chain, new_key1, key1_data, 10, &ssize);
	db_move(chain, new_key1 + 4, new_key1 + 6, 2, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 1 move failed");
	db_read(chain, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move1_data, ssize) == 0, "chain file key 1 move data failed"); 
	
	db_write(chain, new_key1, key1_data, 10, &ssize);
	db_move(chain, new_key1 + 4, new_key1 + 6, 4, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 2 move failed");
	db_read(chain, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move2_data, ssize) == 0, "chain file key 2 move data failed"); 
	
	db_write(chain, new_key1, key1_data, 10, &ssize);
	db_move(chain, new_key1 + 6, new_key1 + 4, 2, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 3 move failed");
	db_read(chain, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move3_data, ssize) == 0, "chain file key 3 move data failed"); 
	
	db_write(chain, new_key1, key1_data, 10, &ssize);
	db_move(chain, new_key1 + 6, new_key1 + 4, 4, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 4 move failed");
	db_read(chain, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move4_data, ssize) == 0, "chain file key 4 move data failed"); 
	
	/* }}}1 */
	/* delete {{{1 */
	action  = ACTION_CRWD_DELETE;
	hash = hash_new();
	ssize = 10 + 10;
		hash_set(hash, "action", TYPE_INT32,  &action);
		hash_set(hash, "key",    TYPE_INT64,  &new_key1);
		hash_set(hash, "size",   TYPE_INT32,  &ssize);
		
		buffer_cleanup(buffer);
		
		ssize = chain_query(chain, hash, buffer);
			fail_unless(ssize == 0,                                "chain file key 1 delete failed");
		
	hash_free(hash);
	/* }}}1 */


	free(test_chunk);
	buffer_free(buffer);
	setting_destroy(setting);
	chain_destroy(chain);
}
END_TEST
REGISTER_TEST(core, test_backend_file)

