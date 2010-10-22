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
	
	hash_t  settings[] = {
		{ NULL, DATA_HASHT(
			{ "name",      DATA_STRING("file")                   },
			{ "filename",  DATA_STRING("data_backend_addrs.dat") },
			hash_end
		)},
		{ NULL, DATA_HASHT(
			{ "name",      DATA_STRING("blocks-address")     },
			{ "per-level", DATA_INT32(4)                     },
			hash_end
		)},
		hash_end
	};
	
	backend = backend_new("addrs", settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	// create 30 blocks with size VSIZE {{{
	block_vid = 0;
	block_off = 0;
	for(i=0; i<blocks_count; i++){
		hash_t  req_write[] = {
			{ "action",     DATA_INT32(ACTION_CRWD_WRITE) },
			{ "block_vid",  DATA_PTR_INT32(&block_vid)    },
			{ "block_off",  DATA_PTR_INT32(&block_off)    },
			{ "block_size", DATA_INT32(VSIZE)             },
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
		{ "action", DATA_INT32(ACTION_CRWD_COUNT) },
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
			{ "action", DATA_INT32(ACTION_CRWD_READ) },
			{ "offset", DATA_PTR_OFFT(&offset_virt)  },
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
		{ "action",     DATA_INT32(ACTION_CRWD_WRITE) },
		{ "block_vid",  DATA_INT32(3)                 },
		{ "block_size", DATA_INT32(0x110)             },
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
		{ "action",     DATA_INT32(ACTION_CRWD_DELETE) },
		{ "block_vid",  DATA_INT32(2)                  },
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
}
END_TEST
REGISTER_TEST(core, test_backend_blocks_addrs)

