#include "test.h"

START_TEST (test_buffer){
	size_t     size;
	void      *chunk;
	void      *chunk2;
	void      *test;
	buffer_t  *buffer;
	
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
}
END_TEST
REGISTER_TEST(core, test_buffer)


