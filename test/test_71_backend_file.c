START_TEST (test_backend_file){
	size_t                 count;
	ssize_t                ssize;
	off_t                  new_key1, new_key2;
	backend_t             *backend;
	
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
	
	
	ssize = backend_stdcall_create(backend, &new_key1, 10);
		fail_unless(ssize == 0, "backend file create key1 failed");	
	ssize = backend_stdcall_create(backend, &new_key2, 10);
		fail_unless(ssize == 0, "backend file create key2 failed");	
		fail_unless(new_key2 - new_key1 == 10,                 "backend file offsets invalid");
	
	char  key1_data[]  = "test167890";
	char  key2_data[]  = "test267890";
	
	char  move1_data[] = "test161690";
	char  move2_data[] = "test161678";
	char  move3_data[] = "test787890";
	char  move4_data[] = "test789090";
	
	char *test_chunk = malloc(100);

	/* write {{{1 */
	ssize = backend_stdcall_write(backend, new_key1, key1_data, 10);
		fail_unless(ssize == 10,                  "backend file key 1 set failed");
	
	ssize = backend_stdcall_write(backend, new_key2, key2_data, 10);
		fail_unless(ssize == 10,                  "backend file key 2 set failed");
		
	/* }}}1 */
	/* read {{{1 */
	ssize = backend_stdcall_read(backend, new_key1, test_chunk, 10); 
		fail_unless(ssize == 10,                               "backend file key 1 get failed");
		fail_unless(memcmp(test_chunk, key1_data, ssize) == 0, "backend file key 1 get data failed"); 
	
	ssize = backend_stdcall_read(backend, new_key2, test_chunk, 10); 
		fail_unless(ssize == 10,                               "backend file key 2 get failed");
		fail_unless(memcmp(test_chunk, key2_data, ssize) == 0, "backend file key 2 get data failed"); 

	/* }}}1 */
	/* count {{{1 */
	ssize = backend_stdcall_count(backend, &count);
		fail_unless(ssize > 0,                                "backend file count failed");
		fail_unless( (count / 20) >= 1,                       "backend file count failed");
		
	/* }}}1 */
	/* move {{{1 */
	ssize = backend_stdcall_write(backend, new_key1, key1_data, 10);
	ssize = backend_stdcall_move(backend, new_key1 + 4, new_key1 + 6, 2);
		fail_unless(ssize == 0,                                 "backend file key 1 move failed");
	ssize = backend_stdcall_read(backend, new_key1, test_chunk, 10); 
		fail_unless(memcmp(test_chunk, move1_data, ssize) == 0, "backend file key 1 move data failed"); 
	
	ssize = backend_stdcall_write(backend, new_key1, key1_data, 10);
	ssize = backend_stdcall_move(backend, new_key1 + 4, new_key1 + 6, 4);
		fail_unless(ssize == 0,                                 "backend file key 2 move failed");
	ssize = backend_stdcall_read(backend, new_key1, test_chunk, 10); 
		fail_unless(memcmp(test_chunk, move2_data, ssize) == 0, "backend file key 2 move data failed"); 
	
	ssize = backend_stdcall_write(backend, new_key1, key1_data, 10);
	ssize = backend_stdcall_move(backend, new_key1 + 6, new_key1 + 4, 2);
		fail_unless(ssize == 0,                                 "backend file key 3 move failed");
	ssize = backend_stdcall_read(backend, new_key1, test_chunk, 10); 
		fail_unless(memcmp(test_chunk, move3_data, ssize) == 0, "backend file key 3 move data failed"); 
	
	ssize = backend_stdcall_write(backend, new_key1, key1_data, 10);
	ssize = backend_stdcall_move(backend, new_key1 + 6, new_key1 + 4, 4);
		fail_unless(ssize == 0,                                 "backend file key 4 move failed");
	ssize = backend_stdcall_read(backend, new_key1, test_chunk, 10); 
		fail_unless(memcmp(test_chunk, move4_data, ssize) == 0, "backend file key 4 move data failed"); 
	
	/* }}}1 */
	
	/* delete {{{1 */
	ssize = backend_stdcall_delete(backend, new_key1, 10 + 10);
		fail_unless(ssize == 0,                                "backend file key 1 delete failed");
	/* }}}1 */
	
	free(test_chunk);
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_file)

