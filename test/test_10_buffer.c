
START_TEST (test_buffer){
	buffer_t  *buffer;
	char       data[] = "1234567890";
	char      *testb  = calloc(1, 100);
	size_t     size;
	
	buffer = buffer_alloc();
		fail_unless(buffer != NULL, "buffer create failed");
	size = buffer_get_size(buffer);
		fail_unless(size == 0, "buffer size failed");
	
	buffer_add_head_chunk(buffer, 100);
	size   = buffer_get_size(buffer);
		fail_unless(size == 100, "buffer add head chunk failed");
	buffer_add_tail_chunk(buffer, 50);
	size   = buffer_get_size(buffer);
		fail_unless(size == 150, "buffer add tail chunk failed");
	
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
	
	char      data1[] = "test1234tezt5678";
	char      data2[] = "testtezt56781234";
	buffer_t  data1_buffer, data2_buffer;
	
	buffer_init_from_bare(&data1_buffer, &data1, sizeof(data1));
	buffer_init_from_bare(&data2_buffer, &data2, sizeof(data2));
	
	// TODO check chunked buffers
	
	fail_unless( buffer_memcmp(&data1_buffer, 0, &data2_buffer, 0, 4) == 0, "buffer_memcmp failed (test <=> test)");
	fail_unless( buffer_memcmp(&data1_buffer, 8, &data2_buffer, 4, 4) == 0, "buffer_memcmp failed (tezt <=> tezt)");
	fail_unless( buffer_memcmp(&data1_buffer, 4, &data2_buffer, 8, 4) != 0, "buffer_memcmp failed (1234 <=> 5678)");
	
	buffer_destroy(&data1_buffer);
	buffer_destroy(&data2_buffer);
	
	free(testb);
}
END_TEST
REGISTER_TEST(core, test_buffer)


