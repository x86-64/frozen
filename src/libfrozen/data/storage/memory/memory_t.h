#ifndef DATA_MEMORY_T_H
#define DATA_MEMORY_T_H

#define DATA_MEMORYT(_mem) {TYPE_MEMORYT, _mem}
#define DEREF_TYPE_MEMORYT(_data) (memory_t *)((_data)->ptr)
#define REF_TYPE_MEMORYT(_dt) _dt
#define HAVEBUFF_TYPE_MEMORYT 0
	
typedef enum memory_t_type {
	MEMORY_FREED = 0,
	
	MEMORY_EXACT,         // memory size is exact to hold data
	MEMORY_PAGE,          // memory expand granularity is page
	MEMORY_DOUBLE         // memory expands by doubling current size
} memory_t_type;

typedef enum memory_t_alloc {
	ALLOC_FREED = 0,
	ALLOC_MALLOC,
} memory_t_alloc;

typedef struct memory_t {
	void           *ptr;
	uintmax_t       ptr_size;
	uintmax_t       exact_size;
	uintmax_t       page_size;
	
	int             zero;
	memory_t_type   type;
	memory_t_alloc  alloc;
} memory_t;

API memory_t_type  memory_string_to_type(char *string);
API memory_t_alloc memory_string_to_alloc(char *string);
API void     memory_new(memory_t *memory, memory_t_type type, memory_t_alloc alloc, uintmax_t page_size, int zero); 
API void     memory_free(memory_t *memory); 
API ssize_t  memory_size(memory_t *memory, uintmax_t *size); 
API ssize_t  memory_resize(memory_t *memory, uintmax_t new_size); 
API ssize_t  memory_grow(memory_t *memory, uintmax_t size, off_t *pointer); 
API ssize_t  memory_shrink(memory_t *memory, uintmax_t size); 
API ssize_t  memory_translate(memory_t *memory, off_t pointer, uintmax_t size, void **pointer_out, uintmax_t *size_out); 

API size_t   memory_read   (memory_t *memory, off_t offset, void *buffer, size_t buffer_size);
API size_t   memory_write  (memory_t *memory, off_t offset, void *buffer, size_t buffer_size);

#endif
