#include "test.h"

void  db_read(chain_t *chain, off_t key, size_t size, char *buf, ssize_t *ssize){
	buffer_t       *buffer = buffer_alloc();
	unsigned int    action;
	request_t      *request;
	hash_builder_t *builder;
	char           *test_chunk;
	
	action  = ACTION_CRWD_READ;
	builder = hash_builder_new(3);
		hash_builder_add_data(&builder, "action", TYPE_INT32,  &action);
		hash_builder_add_data(&builder, "key",    TYPE_INT64,  &key);
		hash_builder_add_data(&builder, "size",   TYPE_INT32,  &size);
		
		request = hash_builder_get_hash(builder);
		
		*ssize = chain_query(chain, request, buffer);
		if(*ssize > 0){
			test_chunk = buffer_get_first_chunk(buffer);
			memcpy(buf, test_chunk, *ssize);
		}
		
	hash_builder_free(builder);
	
	buffer_free(buffer);
}

void  db_write(chain_t *chain, off_t key, char *buf, ssize_t *ssize){
	buffer_t       *buffer = buffer_alloc();
	unsigned int    action;
	request_t      *request;
	hash_builder_t *builder;
	char           *test_chunk;
	
	action  = ACTION_CRWD_WRITE;
	builder = hash_builder_new(3);
		hash_builder_add_data(&builder, "action", TYPE_INT32,  &action);
		hash_builder_add_data(&builder, "key",    TYPE_INT64,  &key);
		hash_builder_add_data(&builder, "value",  TYPE_BINARY, buf);
		
		request = hash_builder_get_hash(builder);
		
		*ssize = chain_query(chain, request, buffer);
	
	hash_builder_free(builder);
	buffer_free(buffer);
}

void  db_move(chain_t *chain, off_t key_from, off_t key_to, size_t key_size, ssize_t *ssize){
	buffer_t       *buffer = buffer_alloc();
	unsigned int    action;
	request_t      *request;
	hash_builder_t *builder;
	char           *test_chunk;
	
	action  = ACTION_CRWD_MOVE;
	builder = hash_builder_new(4);
		hash_builder_add_data(&builder, "action",   TYPE_INT32, &action);
		hash_builder_add_data(&builder, "key_from", TYPE_INT64, &key_from); 
		hash_builder_add_data(&builder, "key_to",   TYPE_INT64, &key_to); 
		hash_builder_add_data(&builder, "size",     TYPE_INT32, &key_size);
		
		request = hash_builder_get_hash(builder);
		
		*ssize = chain_query(chain, request, buffer);
		
	hash_builder_free(builder);
	buffer_free(buffer);
}

START_TEST (test_backend_file){
	int             ret;
	ssize_t         ssize;
	off_t           new_key1, new_key2;
	buffer_t       *buffer  = buffer_alloc();
	request_t      *request;
	hash_builder_t *builder;
	unsigned int    action;

	chain_t *chain = chain_new("file");
		fail_unless(chain != NULL, "chain 'file' not created");
	
	setting_t *setting = setting_new();
	setting_set_child_string(global_settings, "homedir",  ".");
	setting_set_child_string(setting,         "filename", "data_backend_file");
	
	ret = chain_configure(chain, setting);
		fail_unless(ret == 0, "chain 'file' configuration failed");
	
	action  = ACTION_CRWD_CREATE;
	builder = hash_builder_new(2);
		hash_builder_add_data(&builder, "action", TYPE_INT32, &action);
		hash_builder_add_data(&builder, "size",   TYPE_INT32, "\x0e\x00\x00\x00");
		
		request = hash_builder_get_hash(builder);
		
		if( (ssize = chain_query(chain, request, buffer)) != sizeof(off_t) )
			fail("chain file create key1 failed");	
		buffer_read(buffer, ssize, new_key1 = *(off_t *)chunk; break);
		
		if( (ssize = chain_query(chain, request, buffer)) != sizeof(off_t) )
			fail("chain file create key2 failed");	
		buffer_read(buffer, ssize, new_key2 = *(off_t *)chunk; break);
			fail_unless(new_key2 - new_key1 == 14,                 "chain file offsets invalid");
	hash_builder_free(builder);
	
	char  key1_data[]  = "\x0e\x00\x00\x00test167890";
	char  key2_data[]  = "\x0e\x00\x00\x00test267890";
	
	char  move1_data[] = "\x0e\x00\x00\x00test161690";
	char  move2_data[] = "\x0e\x00\x00\x00test161678";
	char  move3_data[] = "\x0e\x00\x00\x00test787890";
	char  move4_data[] = "\x0e\x00\x00\x00test789090";
	
	char *test_chunk = malloc(100);

	/* write {{{1 */
	db_write(chain, new_key1, key1_data, &ssize);
		fail_unless(ssize == 14,                  "chain file key 1 set failed");
	
	db_write(chain, new_key2, key2_data, &ssize);
		fail_unless(ssize == 14,                  "chain file key 2 set failed");
		
	/* }}}1 */
	/* read {{{1 */
	db_read(chain, new_key1, 14, test_chunk, &ssize); 
		fail_unless(ssize == 14,                               "chain file key 1 get failed");
		fail_unless(memcmp(test_chunk, key1_data, ssize) == 0, "chain file key 1 get data failed"); 
	
	db_read(chain, new_key2, 14, test_chunk, &ssize); 
		fail_unless(ssize == 14,                               "chain file key 2 get failed");
		fail_unless(memcmp(test_chunk, key2_data, ssize) == 0, "chain file key 2 get data failed"); 

	/* }}}1 */
	/* delete {{{1 */
	action  = ACTION_CRWD_DELETE;
	builder = hash_builder_new(3);
		hash_builder_add_data(&builder, "action", TYPE_INT32,  &action);
		hash_builder_add_data(&builder, "key",    TYPE_INT64,  &new_key1);
		hash_builder_add_data(&builder, "size",   TYPE_INT32, "\x0e\x00\x00\x00");
		
		request = hash_builder_get_hash(builder);
		buffer_remove_chunks(buffer);
		
		ssize = chain_query(chain, request, buffer);
			fail_unless(ssize == 0,                                "chain file key 1 delete failed");
		
	hash_builder_free(builder);
	
	action  = ACTION_CRWD_DELETE;
	builder = hash_builder_new(3);
		hash_builder_add_data(&builder, "action", TYPE_INT32,  &action);
		hash_builder_add_data(&builder, "key",    TYPE_INT64,  &new_key2);
		hash_builder_add_data(&builder, "size",   TYPE_INT32, "\x0e\x00\x00\x00");
		
		request = hash_builder_get_hash(builder);
		buffer_remove_chunks(buffer);
		
		ssize = chain_query(chain, request, buffer);
			fail_unless(ssize == 0,                                "chain file key 2 delete failed");
		
	hash_builder_free(builder);
	/* }}}1 */
	/* count {{{1 */
	size_t  count;
	
	action  = ACTION_CRWD_COUNT;
	builder = hash_builder_new(3);
		hash_builder_add_data(&builder, "action", TYPE_INT32, &action);
		
		request = hash_builder_get_hash(builder);
		buffer_remove_chunks(buffer);
		
		ssize = chain_query(chain, request, buffer);
			fail_unless(ssize > 0,                                "chain file count failed");
		
		buffer_read(buffer, ssize, count = *(size_t *)chunk; break);
			fail_unless( (count / 28) >= 1,                       "chain file count failed");
		
	hash_builder_free(builder);
	/* }}}1 */
	/* move {{{1 */
	db_write(chain, new_key1, key1_data, &ssize);
	db_move(chain, new_key1 + 8, new_key1 + 10, 2, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 1 move failed");
	db_read(chain, new_key1, 14, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move1_data, ssize) == 0, "chain file key 1 move data failed"); 
	
	db_write(chain, new_key1, key1_data, &ssize);
	db_move(chain, new_key1 + 8, new_key1 + 10, 4, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 2 move failed");
	db_read(chain, new_key1, 14, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move2_data, ssize) == 0, "chain file key 2 move data failed"); 
	
	db_write(chain, new_key1, key1_data, &ssize);
	db_move(chain, new_key1 + 10, new_key1 + 8, 2, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 3 move failed");
	db_read(chain, new_key1, 14, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move3_data, ssize) == 0, "chain file key 3 move data failed"); 
	
	db_write(chain, new_key1, key1_data, &ssize);
	db_move(chain, new_key1 + 10, new_key1 + 8, 4, &ssize);
		fail_unless(ssize == 0,                                 "chain file key 4 move failed");
	db_read(chain, new_key1, 14, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move4_data, ssize) == 0, "chain file key 4 move data failed"); 
	
	/* }}}1 */
	
	free(test_chunk);
	buffer_free(buffer);
	setting_destroy(setting);
	chain_destroy(chain);
}
END_TEST
REGISTER_TEST(core, test_backend_file)

