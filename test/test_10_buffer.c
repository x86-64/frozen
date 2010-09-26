
START_TEST (test_buffer){
	size_t     size;
	void      *chunk;
	void      *chunk2;
	void      *test;
	buffer_t  *buffer;
	char       data[] = "1234567890";
	char      *testb  = calloc(1, 100);
	
	chunk = chunk_alloc(100);
		fail_unless(chunk != NULL, "chunk create failed");
	size = chunk_get_size(chunk);
		fail_unless(size  == 100, "chunk size invalid");
	
	chunk2 = chunk_alloc(50);
	
	
	buffer = buffer_alloc();
		fail_unless(buffer != NULL, "buffer create failed");
	size = buffer_get_size(buffer);
		fail_unless(size == 0, "buffer size failed");
	
	buffer_add_head_chunk(buffer, chunk);
	size = buffer_get_size(buffer);
		fail_unless(size == 100, "buffer add head chunk failed");
	buffer_add_tail_chunk(buffer, chunk2);
	size = buffer_get_size(buffer);
		fail_unless(size == 150, "buffer add tail chunk failed");
	
	test = chunk_next(chunk);
		fail_unless(test == chunk2, "chunk->next failed");
	
	buffer_delete_chunk(buffer, chunk);
	size = buffer_get_size(buffer);
		fail_unless(size == 50, "buffer delete chunk failed");
	
	buffer_free(buffer);
	buffer = buffer_alloc();
	
	buffer_write(buffer, 0, &data, 1);
	buffer_write(buffer, 0, &data, 2);
	buffer_write(buffer, 0, &data, 3);
	buffer_write(buffer, 0, &data, 4);    // 1234
	buffer_write(buffer, 4, &data[4], 6); // 567890
	buffer_write(buffer, 2, &data[2], 3); // 345
	
	buffer_read(buffer, 0, testb, 10);
		fail_unless(memcmp(testb, &data, 10) == 0, "buffer chunked write-read failed");
	
	buffer_free(buffer);
	free(testb);
}
END_TEST
REGISTER_TEST(core, test_buffer)


