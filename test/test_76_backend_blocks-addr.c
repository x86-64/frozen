#include "test.h"

#define VSIZE 0x100
#define RSIZE 0x1000

START_TEST (test_backend_blocks_addrs){
	int           i;
	ssize_t       ssize;
	size_t        count;
	size_t        blocks_count = 30;
	size_t        match_size   = blocks_count * VSIZE;
	unsigned int  block_vid;
	unsigned int  block_off;
	off_t         offset_virt, offset_real;
	backend_t    *backend;
	buffer_t     *buffer = buffer_alloc();
	
	setting_set_child_string(global_settings, "homedir", ".");
	
	setting_t *s_backend = setting_new_named("backend");
		setting_t *s_bfile = setting_new();
			setting_set_child_string(s_bfile, "name",        "file");
			setting_set_child_string(s_bfile, "filename",    "data_backend_addrs");
		setting_t *s_bloca = setting_new();
			setting_set_child_string(s_bloca, "name",        "blocks-address");
			setting_set_child_string(s_bloca, "per-level",   "4");
	setting_set_child_setting(s_backend, s_bfile);
	setting_set_child_setting(s_backend, s_bloca);
	
	backend = backend_new("addrs", s_backend);
		fail_unless(backend != NULL, "backend creation failed");
	
	// create 30 blocks with size VSIZE {{{
	block_vid = 0;
	block_off = 0;
	for(i=0; i<blocks_count; i++){
		hash_t  req_write[] = {
			{ TYPE_INT32, "action",     (int []){ ACTION_CRWD_WRITE }, sizeof(unsigned int) },
			{ TYPE_INT32, "block_vid",  &block_vid,                    sizeof(unsigned int) },
			{ TYPE_INT32, "block_size", (int []){ VSIZE             }, sizeof(unsigned int) },
			{ TYPE_INT32, "block_off",  &block_off,                    sizeof(unsigned int) },
			hash_end
		};
		block_off += RSIZE;
		
		ssize = backend_query(backend, req_write, NULL);
			fail_unless(ssize == 0, "chain 'blocks-addressing' insert block failed");
	}
	// }}}
	
	count = 0;
	
	// count blocks {{{
	hash_t  req_count[] = {
		{ TYPE_INT32, "action",     (int []){ ACTION_CRWD_COUNT }, sizeof(unsigned int) },
		hash_end
	};
	if( (ssize = backend_query(backend, req_count, buffer)) > 0)
		buffer_read(buffer, 0, &count, MIN(ssize, sizeof(count))); 
	
		fail_unless(count == match_size, "chain 'blocks-addressing' tree size invalid");
	
	// }}}
	// check offsets {{{
	buffer_cleanup(buffer);
	
	for(offset_virt=0; offset_virt<match_size; offset_virt++){
		hash_t  req_read[] = {
			{ TYPE_INT32, "action",     (int []){ ACTION_CRWD_READ }, sizeof(unsigned int) },
			{ TYPE_INT64, "offset",     &offset_virt,                 sizeof(off_t)        },
			hash_end
		};
		if( (ssize = backend_query(backend, req_read, buffer)) > 0){
			buffer_read(buffer, 0, &offset_real, MIN(ssize, sizeof(offset_real))); 
		}
			fail_unless(ssize >= sizeof(off_t),     "chain 'blocks-addressing' read offset failed");
			fail_unless(
				(blocks_count - (offset_virt / VSIZE)) * RSIZE + (offset_virt % VSIZE) == offset_real,
				"chain 'blocks-addressing' read offset data failed"
			);
	}
	// }}}
	
	// resize block {{{
	block_vid = 3;
	hash_t  req_write2[] = {
		{ TYPE_INT32, "action",     (int []){ ACTION_CRWD_WRITE }, sizeof(unsigned int) },
		{ TYPE_INT32, "block_vid",  (int []){ 3                 }, sizeof(unsigned int) },
		{ TYPE_INT32, "block_size", (int []){ 0x110             }, sizeof(unsigned int) },
		hash_end
	};
	ssize = backend_query(backend, req_write2, NULL);
		fail_unless(ssize == 0, "chain 'blocks-adressing' block_resize failed");
	// }}}
	// check overall size {{{
	if( (ssize = backend_query(backend, req_count, buffer)) > 0)
		buffer_read(buffer, 0, &count, MIN(ssize, sizeof(count)));
	
		fail_unless(count == match_size + 0x10, "chain 'blocks-adressing' block_resize tree size invalid");
	// }}}
	// delete block {{{
	hash_t  req_delete[] = {
		{ TYPE_INT32, "action",     (int []){ ACTION_CRWD_DELETE }, sizeof(unsigned int) },
		{ TYPE_INT32, "block_vid",  (int []){ 2                  }, sizeof(unsigned int) },
		hash_end
	};
	ssize = backend_query(backend, req_delete, NULL);
		fail_unless(ssize == 0, "chain 'blocks-adressing' delete failed");
	
	// }}}
	// check overall size {{{
	if( (ssize = backend_query(backend, req_count, buffer)) > 0)
		buffer_read(buffer, 0, &count, MIN(ssize, sizeof(count))); 
		
		fail_unless(count == match_size + 0x10 - VSIZE, "chain 'blocks-adressing' delete tree size invalid");
	// }}}
	
	buffer_free(buffer);
	backend_destroy(backend);
	setting_destroy(s_backend);
}
END_TEST
REGISTER_TEST(core, test_backend_blocks_addrs)

