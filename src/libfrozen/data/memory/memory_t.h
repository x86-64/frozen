#ifndef DATA_MEMORY_T_H
#define DATA_MEMORY_T_H

enum { TYPE_MEMORYT = 18 };
#define DATA_MEMORYT(_mem) {TYPE_MEMORYT, _mem, 0}
	
typedef enum memory_t_type {
	MEMORY_EXACT,         // memory size is exact to hold data
	MEMORY_PAGE,          // memory expand granularity is page
	MEMORY_DOUBLE         // memory expands by doubling current size
} memory_t_type;

typedef enum memory_t_alloc {
	ALLOC_MALLOC,
} memory_t_alloc;

typedef struct memory_t {
	void           *ptr;
	size_t          ptr_size;
	size_t          exact_size;
	size_t          page_size;
	
	int             zero;
	memory_t_type   type;
	memory_t_alloc  alloc;
} memory_t;

API memory_t_type  memory_string_to_type(char *string);
API memory_t_alloc memory_string_to_alloc(char *string);
API void     memory_new(memory_t *memory, memory_t_type type, memory_t_alloc alloc, size_t page_size, int zero); 
API void     memory_free(memory_t *memory); 
API ssize_t  memory_size(memory_t *memory, size_t *size); 
API ssize_t  memory_resize(memory_t *memory, size_t new_size); 
API ssize_t  memory_grow(memory_t *memory, size_t size, off_t *pointer); 
API ssize_t  memory_shrink(memory_t *memory, size_t size); 
API ssize_t  memory_translate(memory_t *memory, off_t pointer, size_t size, void **pointer_out, size_t *size_out); 

#endif
