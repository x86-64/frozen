
void  db_read(backend_t *backend, off_t key, size_t size, char *buf, ssize_t *ssize){
	buffer_t       *buffer = buffer_alloc();
	ssize_t         ret;
	
	hash_t hash[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_READ) },
		{ HK(offset),    DATA_PTR_OFFT(&key)          },
		{ HK(size),   DATA_PTR_SIZET(&size)        },
		{ HK(buffer), DATA_BUFFERT(buffer)         },
		hash_end
	};
	
	ret = backend_query(backend, hash);
	if(ret > 0){
		buffer_read(buffer, 0, buf, ret);
	}
	*ssize = ret;
		
	buffer_free(buffer);
}

void  db_write(backend_t *backend, off_t key, char *buf, unsigned int buf_size, ssize_t *ssize){
	buffer_t        buffer;
	
	buffer_init(&buffer);
	buffer_add_head_raw(&buffer, buf, buf_size);
	
	hash_t hash[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_WRITE) },
		{ HK(offset),    DATA_PTR_OFFT(&key)           },
		{ HK(size),   DATA_PTR_SIZET(&buf_size)     },
		{ HK(buffer), DATA_BUFFERT(&buffer)         },
		hash_end
	};
		
	*ssize = backend_query(backend, hash);
	
	buffer_destroy(&buffer);
}

void  db_move(backend_t *backend, off_t key_from, off_t key_to, size_t key_size, ssize_t *ssize){
	buffer_t buffer;
	
	buffer_init(&buffer);
	
	hash_t hash[] = {
		{ HK(action),   DATA_UINT32T(ACTION_CRWD_MOVE) },
		{ HK(offset_from), DATA_PTR_OFFT(&key_from)     },
		{ HK(offset_to),   DATA_PTR_OFFT(&key_to)       },
		{ HK(size),     DATA_PTR_SIZET(&key_size)    },
		{ HK(buffer),   DATA_BUFFERT(&buffer)        },
		hash_end
	};
	
	*ssize = backend_query(backend, hash);
		
	buffer_destroy(&buffer);
}

START_TEST (test_backend_file){
	ssize_t         ssize;
	off_t           new_key1, new_key2, temp;
	buffer_t       *buffer  = buffer_alloc();
	
	backend_t *backend;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                     },
                        { HK(filename),    DATA_STRING("data_backend_file.dat")    },
                        hash_end
                )},
                hash_end
	};
	
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	hash_t hash_create[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_CREATE) },
		{ HK(size),       DATA_SIZET(10)                 },
		{ HK(offset_out), DATA_PTR_OFFT(&temp)           },
		hash_end
	};
		
	if( (ssize = backend_query(backend, hash_create)) < 0 )
		fail("backend file create key1 failed");	
	new_key1 = temp;
	
	if( (ssize = backend_query(backend, hash_create)) < 0 )
		fail("backend file create key2 failed");	
	new_key2 = temp;
		fail_unless(new_key2 - new_key1 == 10,                 "backend file offsets invalid");
	
	char  key1_data[]  = "test167890";
	char  key2_data[]  = "test267890";
	
	char  move1_data[] = "test161690";
	char  move2_data[] = "test161678";
	char  move3_data[] = "test787890";
	char  move4_data[] = "test789090";
	
	char *test_chunk = malloc(100);

	/* write {{{1 */
	db_write(backend, new_key1, key1_data, 10, &ssize);
		fail_unless(ssize == 10,                  "backend file key 1 set failed");
	
	db_write(backend, new_key2, key2_data, 10, &ssize);
		fail_unless(ssize == 10,                  "backend file key 2 set failed");
		
	/* }}}1 */
	/* read {{{1 */
	db_read(backend, new_key1, 10, test_chunk, &ssize); 
		fail_unless(ssize == 10,                               "backend file key 1 get failed");
		fail_unless(memcmp(test_chunk, key1_data, ssize) == 0, "backend file key 1 get data failed"); 
	
	db_read(backend, new_key2, 10, test_chunk, &ssize); 
		fail_unless(ssize == 10,                               "backend file key 2 get failed");
		fail_unless(memcmp(test_chunk, key2_data, ssize) == 0, "backend file key 2 get data failed"); 

	/* }}}1 */
	/* count {{{1 */
	size_t  count;
	
	hash_t hash_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_BUFFERT(buffer)          },
		hash_end
	};
		
	ssize = backend_query(backend, hash_count);
		fail_unless(ssize > 0,                                "backend file count failed");
	
	buffer_read(buffer, 0, &count, MIN(ssize, sizeof(size_t)));
		fail_unless( (count / 20) >= 1,                       "backend file count failed");
		
	/* }}}1 */
	/* move {{{1 */
	db_write(backend, new_key1, key1_data, 10, &ssize);
	db_move(backend, new_key1 + 4, new_key1 + 6, 2, &ssize);
		fail_unless(ssize == 0,                                 "backend file key 1 move failed");
	db_read(backend, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move1_data, ssize) == 0, "backend file key 1 move data failed"); 
	
	db_write(backend, new_key1, key1_data, 10, &ssize);
	db_move(backend, new_key1 + 4, new_key1 + 6, 4, &ssize);
		fail_unless(ssize == 0,                                 "backend file key 2 move failed");
	db_read(backend, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move2_data, ssize) == 0, "backend file key 2 move data failed"); 
	
	db_write(backend, new_key1, key1_data, 10, &ssize);
	db_move(backend, new_key1 + 6, new_key1 + 4, 2, &ssize);
		fail_unless(ssize == 0,                                 "backend file key 3 move failed");
	db_read(backend, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move3_data, ssize) == 0, "backend file key 3 move data failed"); 
	
	db_write(backend, new_key1, key1_data, 10, &ssize);
	db_move(backend, new_key1 + 6, new_key1 + 4, 4, &ssize);
		fail_unless(ssize == 0,                                 "backend file key 4 move failed");
	db_read(backend, new_key1, 10, test_chunk, &ssize); 
		fail_unless(memcmp(test_chunk, move4_data, ssize) == 0, "backend file key 4 move data failed"); 
	
	/* }}}1 */
	
	/* delete {{{1 */
	ssize = 10 + 10;
	
	hash_t hash_delete[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_DELETE) },
		{ HK(offset),    DATA_PTR_OFFT(&new_key1)       },
		{ HK(size),   DATA_PTR_SIZET(&ssize)         },
		{ HK(buffer), DATA_BUFFERT(buffer)           },
		hash_end
	};
	
	ssize = backend_query(backend, hash_delete);
		fail_unless(ssize == 0,                                "backend file key 1 delete failed");
		
	/* }}}1 */
	
	free(test_chunk);
	buffer_free(buffer);
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_file)

