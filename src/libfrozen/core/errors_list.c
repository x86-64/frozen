
typedef struct err_item {
        intmax_t    errnum;
        const char *errmsg;
} err_item;
static err_item errs_list[] = {
 { -61443, "src/libfrozen/backends/balancer/balancer.c: no childs" },
 { -61442, "src/libfrozen/backends/balancer/balancer.c: fork failed" },
 { -61441, "src/libfrozen/backends/balancer/balancer.c: no memory" },
 { -61440, "src/libfrozen/backends/balancer/balancer.c: calloc returns null" },
 { -57348, "src/libfrozen/backends/insert_sort/insert_sort.m4: no key_out supplied" },
 { -57348, "src/libfrozen/backends/insert_sort/insert_sort.c: no key_out supplied" },
 { -57347, "src/libfrozen/backends/insert_sort/insert_sort.m4: no buffer supplied" },
 { -57347, "src/libfrozen/backends/insert_sort/insert_sort.c: no buffer supplied" },
 { -57346, "src/libfrozen/backends/insert_sort/insert_sort.m4: backend insert-sort engine not found" },
 { -57346, "src/libfrozen/backends/insert_sort/insert_sort.c: backend insert-sort engine not found" },
 { -57345, "src/libfrozen/backends/insert_sort/insert_sort.m4: backend insert-sort parameter engine not supplied" },
 { -57345, "src/libfrozen/backends/insert_sort/insert_sort.c: backend insert-sort parameter engine not supplied" },
 { -57344, "src/libfrozen/backends/insert_sort/insert_sort.m4: calloc failed" },
 { -57344, "src/libfrozen/backends/insert_sort/insert_sort.c: calloc failed" },
 { -53256, "src/libfrozen/backends/rewrite/rewrite.c: backend_acquire failed" },
 { -53255, "src/libfrozen/backends/rewrite/rewrite.c: alloca data invalid" },
 { -53254, "src/libfrozen/backends/rewrite/rewrite.c: arithmetic failed" },
 { -53253, "src/libfrozen/backends/rewrite/rewrite.c: pass failed" },
 { -53252, "src/libfrozen/backends/rewrite/rewrite.c: no cmp" },
 { -53251, "src/libfrozen/backends/rewrite/rewrite.c: no cmp" },
 { -53250, "src/libfrozen/backends/rewrite/rewrite.c: script parse failed" },
 { -53249, "src/libfrozen/backends/rewrite/rewrite.c: script not supplied" },
 { -53248, "src/libfrozen/backends/rewrite/rewrite.c: calloc failed" },
 { -49172, "src/libfrozen/backends/mphf/mphf_chm_imp.c: delete not supported" },
 { -49171, "src/libfrozen/backends/mphf/mphf_chm_imp.c: v array delete failed" },
 { -49170, "src/libfrozen/backends/mphf/mphf_chm_imp.c: e array delete failed" },
 { -49169, "src/libfrozen/backends/mphf/mphf_chm_imp.c: g array delete failed" },
 { -49168, "src/libfrozen/backends/mphf/mphf_chm_imp.c: v array fill failed" },
 { -49167, "src/libfrozen/backends/mphf/mphf_chm_imp.c: v array create failed" },
 { -49166, "src/libfrozen/backends/mphf/mphf_chm_imp.c: g array fill failed" },
 { -49165, "src/libfrozen/backends/mphf/mphf_chm_imp.c: g array write failed" },
 { -49164, "src/libfrozen/backends/mphf/mphf_chm_imp.c: g array create failed" },
 { -49163, "src/libfrozen/backends/mphf/mphf_chm_imp.c: recalc db inconsistency" },
 { -49162, "src/libfrozen/backends/mphf/mphf_chm_imp.c: set_g failed" },
 { -49161, "src/libfrozen/backends/mphf/mphf_chm_imp.c: get_g failed" },
 { -49160, "src/libfrozen/backends/mphf/mphf_chm_imp.c: edge not found" },
 { -49159, "src/libfrozen/backends/mphf/mphf_chm_imp.c: find_edge db inconsistency" },
 { -49158, "src/libfrozen/backends/mphf/mphf_chm_imp.c: set_edge failed" },
 { -49157, "src/libfrozen/backends/mphf/mphf_chm_imp.c: get_edge failed" },
 { -49156, "src/libfrozen/backends/mphf/mphf_chm_imp.c: set first failed" },
 { -49155, "src/libfrozen/backends/mphf/mphf_chm_imp.c: get_first failed" },
 { -49154, "src/libfrozen/backends/mphf/mphf_chm_imp.c: new_edge_id failed" },
 { -49153, "src/libfrozen/backends/mphf/mphf_chm_imp.c: mphf is read-only" },
 { -49152, "src/libfrozen/backends/mphf/mphf_chm_imp.c: backend chm_imp parameter backend_g invalid" },
 { -45062, "src/libfrozen/backends/mphf/mphf.c: backend mphf parameter hash invalid" },
 { -45061, "src/libfrozen/backends/mphf/mphf.c: backend mphf parameter mphf_type invalid or not supplied" },
 { -45060, "src/libfrozen/backends/mphf/mphf.c: calloc failed" },
 { -45059, "src/libfrozen/backends/mphf/mphf.c: no keyid or key found in item" },
 { -45058, "src/libfrozen/backends/mphf/mphf.c: failed call" },
 { -45057, "src/libfrozen/backends/mphf/mphf.c: mphf full" },
 { -45056, "src/libfrozen/backends/mphf/mphf.c: malloc failed" },
 { -40968, "src/libfrozen/backends/ipc/ipc_shmem.c: strange error 3" },
 { -40967, "src/libfrozen/backends/ipc/ipc_shmem.c: strange error" },
 { -40966, "src/libfrozen/backends/ipc/ipc_shmem.c: pthread_create failed" },
 { -40965, "src/libfrozen/backends/ipc/ipc_shmem.c: invalid role supplied" },
 { -40964, "src/libfrozen/backends/ipc/ipc_shmem.c: shmat failed" },
 { -40963, "src/libfrozen/backends/ipc/ipc_shmem.c: shmget failed" },
 { -40962, "src/libfrozen/backends/ipc/ipc_shmem.c: no role supplied" },
 { -40961, "src/libfrozen/backends/ipc/ipc_shmem.c: no key supplied" },
 { -40960, "src/libfrozen/backends/ipc/ipc_shmem.c: ipc_shmem_listen dead" },
 { -36865, "src/libfrozen/backends/ipc/ipc.c: backend ipc parameter type invalid" },
 { -36864, "src/libfrozen/backends/ipc/ipc.c: calloc failed" },
 { -32775, "src/libfrozen/backends/allocator/allocator.c: no such function" },
 { -32774, "src/libfrozen/backends/allocator/allocator.c: no size_old supplied" },
 { -32773, "src/libfrozen/backends/allocator/allocator.c: no size supplied" },
 { -32772, "src/libfrozen/backends/allocator/allocator.c: no offset supplied" },
 { -32771, "src/libfrozen/backends/allocator/allocator.c: no function supplied" },
 { -32770, "src/libfrozen/backends/allocator/allocator.c: no size supplied" },
 { -32769, "src/libfrozen/backends/allocator/allocator.c: no offset supplied" },
 { -32768, "src/libfrozen/backends/allocator/allocator.c: calloc failed" },
 { -28673, "src/libfrozen/backends/incapsulate/incapsulate.c: backend incapsulate parameter multiply invalid" },
 { -28672, "src/libfrozen/backends/incapsulate/incapsulate.c: calloc failed" },
 { -24582, "src/libfrozen/backends/structs/structs.c: struct_unpack failed" },
 { -24581, "src/libfrozen/backends/structs/structs.c: hash with keys not supplied" },
 { -24580, "src/libfrozen/backends/structs/structs.c: struct_pack failed" },
 { -24579, "src/libfrozen/backends/structs/structs.c: hash with keys not supplied" },
 { -24578, "src/libfrozen/backends/structs/structs.c: backend struct parameter structure invalid" },
 { -24577, "src/libfrozen/backends/structs/structs.c: backend struct parameter key invalid" },
 { -24576, "src/libfrozen/backends/structs/structs.c: calloc failed" },
 { -20487, "src/libfrozen/backends/cache_append/cache_append.c: memory_size failed" },
 { -20486, "src/libfrozen/backends/cache_append/cache_append.c: count failed" },
 { -20485, "src/libfrozen/backends/cache_append/cache_append.c: memory_grow failed" },
 { -20484, "src/libfrozen/backends/cache_append/cache_append.c: no size supplied" },
 { -20483, "src/libfrozen/backends/cache_append/cache_append.c: memory_size failed" },
 { -20482, "src/libfrozen/backends/cache_append/cache_append.c: count failed" },
 { -20481, "src/libfrozen/backends/cache_append/cache_append.c: size too small" },
 { -20480, "src/libfrozen/backends/cache_append/cache_append.c: calloc failed" },
 { -16388, "src/libfrozen/backends/cache/cache.c: memory_size failed" },
 { -16387, "src/libfrozen/backends/cache/cache.c: memory_grow failed" },
 { -16386, "src/libfrozen/backends/cache/cache.c: no size supplied" },
 { -16385, "src/libfrozen/backends/cache/cache.c: count failed" },
 { -16384, "src/libfrozen/backends/cache/cache.c: calloc failed" },
 { -12292, "src/libfrozen/backends/list/list.c: data_arithmetic failed" },
 { -12291, "src/libfrozen/backends/list/list.c: no size supplied" },
 { -12290, "src/libfrozen/backends/list/list.c: no offset supplied" },
 { -12289, "src/libfrozen/backends/list/list.c: data_arithmetic failed" },
 { -12288, "src/libfrozen/backends/list/list.c: no offset supplied" },
 { -8210, "src/libfrozen/backends/file/file.c: file_update_count failed" },
 { -8209, "src/libfrozen/backends/file/file.c: no buffer supplied" },
 { -8208, "src/libfrozen/backends/file/file.c: io error" },
 { -8207, "src/libfrozen/backends/file/file.c: malloc error" },
 { -8206, "src/libfrozen/backends/file/file.c: size is null" },
 { -8205, "src/libfrozen/backends/file/file.c: file_update_count failed" },
 { -8204, "src/libfrozen/backends/file/file.c: offset_to not supplied" },
 { -8203, "src/libfrozen/backends/file/file.c: offset_from not supplied" },
 { -8202, "src/libfrozen/backends/file/file.c: cant delete not last elements" },
 { -8201, "src/libfrozen/backends/file/file.c: file_update_count failed" },
 { -8200, "src/libfrozen/backends/file/file.c: size not supplied" },
 { -8199, "src/libfrozen/backends/file/file.c: offset not supplied" },
 { -8198, "src/libfrozen/backends/file/file.c: file expand error" },
 { -8197, "src/libfrozen/backends/file/file.c: size not supplied" },
 { -8196, "src/libfrozen/backends/file/file.c: calloc returns null" },
 { -8195, "src/libfrozen/backends/file/file.c: file open() error" },
 { -8194, "src/libfrozen/backends/file/file.c: filepath invalid" },
 { -8193, "src/libfrozen/backends/file/file.c: handle not supplied" },
 { -8192, "src/libfrozen/backends/file/file.c: handle not supplied" },
 { -4102, "src/libfrozen/backends/blocks/blocks.c: no offset_to supplied" },
 { -4101, "src/libfrozen/backends/blocks/blocks.c: no offset_from supplied" },
 { -4100, "src/libfrozen/backends/blocks/blocks.c: no size supplied" },
 { -4100, "src/libfrozen/backends/blocks_addressing/blocks_addressing.c: no block_vid supplied" },
 { -4099, "src/libfrozen/backends/blocks/blocks.c: no offset supplied" },
 { -4099, "src/libfrozen/backends/blocks_addressing/blocks_addressing.c: no offset supplied" },
 { -4098, "src/libfrozen/backends/blocks/blocks.c: chain blocks variable 'backend' invalid" },
 { -4098, "src/libfrozen/backends/blocks_addressing/blocks_addressing.c: no block_size supplied" },
 { -4097, "src/libfrozen/backends/blocks/blocks.c: chain blocks variable 'backend' not set" },
 { -4097, "src/libfrozen/backends/blocks_addressing/blocks_addressing.c: chain blocks-address tree recalc failed" },
 { -4096, "src/libfrozen/backends/mphf/mphf_bdz_imp.c: mphf g_free" },
 { -4096, "src/libfrozen/backends/blocks/blocks.c: chain blocks variable 'block_size' invalid" },
 { -4096, "src/libfrozen/backends/blocks_addressing/blocks_addressing.c: chain blocks-address no memory" },

};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
