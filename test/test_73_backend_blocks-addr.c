#include "test.h"

#define VSIZE 0x100
#define RSIZE 0x1000

START_TEST (test_backend_blocks_addrs){
	int           action, i;
	ssize_t       ssize;
	size_t        count;
	size_t        blocks_count = 30;
	size_t        match_size   = blocks_count * VSIZE;
	unsigned int  block_vid;
	unsigned int  block_off;
	off_t         offset_virt, offset_real;
	backend_t    *backend;
	hash_t       *hash   = hash_new();
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
	action    = ACTION_CRWD_WRITE;
	ssize     = VSIZE;
	block_vid = 0;
	block_off = 0;
		hash_set(hash, "action",     TYPE_INT32, &action);
		hash_set(hash, "block_vid",  TYPE_INT32, &block_vid);
		hash_set(hash, "block_size", TYPE_INT32, &ssize);
	
	for(i=0; i<blocks_count; i++){
		hash_set(hash, "block_off",  TYPE_INT32, &block_off);
		block_off += RSIZE;
		
		ssize = backend_query(backend, hash, NULL);
			fail_unless(ssize == 0, "chain 'blocks-addressing' insert block failed");
	}
	// }}}
	
	count = 0;
	
	// count blocks {{{
	action    = ACTION_CRWD_COUNT; 
		hash_set(hash, "action",     TYPE_INT32, &action);	
		if( (ssize = backend_query(backend, hash, buffer)) > 0)
			buffer_read_flat(buffer, ssize, &count, sizeof(count)); 
		
		fail_unless(count == match_size, "chain 'blocks-addressing' tree size invalid");
	// }}}
	
	// check offsets {{{
	buffer_remove_chunks(buffer);
	
	action    = ACTION_CRWD_READ;
		hash_set(hash, "action",     TYPE_INT32, &action);
	
	for(offset_virt=0; offset_virt<match_size; offset_virt++){
		hash_set(hash, "offset",     TYPE_INT64, &offset_virt);
		
		if( (ssize = backend_query(backend, hash, buffer)) > 0){
			buffer_read_flat(buffer, ssize, &offset_real, sizeof(offset_real)); 
		}
			fail_unless(ssize == sizeof(off_t),     "chain 'blocks-addressing' read offset failed");
			fail_unless(
				(blocks_count - (offset_virt / VSIZE) -1) * RSIZE + (offset_virt % VSIZE) == offset_real,
				"chain 'blocks-addressing' read offset data failed"
			);
	}
	// }}}

	hash_empty(hash);
	
	// resize block {{{
	action    = ACTION_CRWD_WRITE;
	ssize     = 0x110;
	block_vid = 3;
		hash_set(hash, "action",     TYPE_INT32, &action);
		hash_set(hash, "block_vid",  TYPE_INT32, &block_vid);
		hash_set(hash, "block_size", TYPE_INT32, &ssize);
	
		ssize = backend_query(backend, hash, NULL);
			fail_unless(ssize == 0, "chain 'blocks-adressing' block_resize failed");
	// }}}
	// check overall size {{{
	action    = ACTION_CRWD_COUNT; 
		hash_set(hash, "action",     TYPE_INT32, &action);	
		if( (ssize = backend_query(backend, hash, buffer)) > 0)
			buffer_read_flat(buffer, ssize, &count, sizeof(count)); 
		
		fail_unless(count == match_size + 0x10, "chain 'blocks-adressing' block_resize tree size invalid");
	// }}}
	// delete block {{{
	action    = ACTION_CRWD_DELETE;
	block_vid = 2;
		hash_set(hash, "action",     TYPE_INT32, &action);
		hash_set(hash, "block_vid",  TYPE_INT32, &block_vid);
		
		ssize = backend_query(backend, hash, NULL);
			fail_unless(ssize == 0, "chain 'blocks-adressing' delete failed");
	// }}}
	// check overall size {{{
	action    = ACTION_CRWD_COUNT; 
		hash_set(hash, "action",     TYPE_INT32, &action);	
		if( (ssize = backend_query(backend, hash, buffer)) > 0)
			buffer_read_flat(buffer, ssize, &count, sizeof(count)); 
		
		fail_unless(count == match_size + 0x10 - VSIZE, "chain 'blocks-adressing' delete tree size invalid");
	// }}}

	backend_destory(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_blocks_addrs)

