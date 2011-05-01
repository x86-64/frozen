#include <libfrozen.h>
#include <memory_t.h>
#include <uint/off_t.h>
#include <uint/size_t.h>

memory_t_type  memory_string_to_type(char *string){ // {{{
	if(strcmp(string, "exact") == 0)  return MEMORY_EXACT;
	if(strcmp(string, "page")  == 0)  return MEMORY_PAGE;
	if(strcmp(string, "double") == 0) return MEMORY_DOUBLE;
	return -1;
} // }}}
memory_t_alloc memory_string_to_alloc(char *string){ // {{{
	if(strcmp(string, "malloc") == 0) return ALLOC_MALLOC;
	return -1;
} // }}}
void     memory_new(memory_t *memory, memory_t_type type, memory_t_alloc alloc, uintmax_t page_size, int zero){ // {{{
	memory->ptr       = NULL;
	memory->ptr_size  = 0;
	memory->type      = type;
	memory->alloc     = alloc;
	memory->page_size = page_size;
	memory->zero      = zero;
} // }}}
void     memory_free(memory_t *memory){ // {{{
	if(memory->ptr)
		free(memory->ptr);
	memset(memory, 0, sizeof(memory_t));
} // }}}
ssize_t  memory_size(memory_t *memory, uintmax_t *size){ // {{{
	*size = memory->exact_size;
	return 0;
} // }}}
ssize_t  memory_resize(memory_t *memory, uintmax_t new_size){ // {{{
	uintmax_t t;
	
	memory->exact_size = new_size;
	switch(memory->type){
		case MEMORY_EXACT:
			memory->ptr_size = new_size;
			break;
		case MEMORY_PAGE:
			t = ((new_size / memory->page_size) + 1) * memory->page_size;
			
			if(t == memory->ptr_size)
				return 0;
			
			memory->ptr_size = t;
			break;
		case MEMORY_DOUBLE:
			for(t = 1; t < new_size; t <<= 1);
			
			if(t == memory->ptr_size)
				return 0;
			
			memory->ptr_size = t;
			break;
		case MEMORY_FREED:
			return -EFAULT;
	};
	
	switch(memory->alloc){
		case ALLOC_MALLOC:
			if( (memory->ptr = realloc(memory->ptr, memory->ptr_size)) == NULL)
				goto error;
			break;
		default:
			goto error;
	};
	
	return 0;
error:
	memory_free(memory);
	return -EFAULT;
} // }}}
ssize_t  memory_grow(memory_t *memory, uintmax_t size, off_t *pointer){ // {{{
	ssize_t                ret;
	uintmax_t              new_offset;
	
	new_offset = memory->exact_size;
	
	if(__MAX(uintmax_t) - size <= new_offset)
		return -EINVAL;
	
	if(pointer)
		*pointer = new_offset;
	
	ret = memory_resize(memory, (new_offset + size));
	
	if(ret == 0 && memory->zero){
		memset(memory->ptr + new_offset, 0, size);
	}
	return ret;
} // }}}
ssize_t  memory_shrink(memory_t *memory, uintmax_t size){ // {{{
	if(memory->exact_size < size)
		return -EINVAL;
	
	return memory_resize(memory, (memory->exact_size - size));
} // }}}
ssize_t  memory_translate(memory_t *memory, off_t pointer, uintmax_t size, void **pointer_out, uintmax_t *size_out){ // {{{
	switch(memory->alloc){
		case ALLOC_MALLOC:
			// TODO IMPORTANT SECURITY snap ptr+size to mem
			if(pointer >= memory->exact_size)
				return -EINVAL;
			
			size = MIN(size, memory->exact_size - pointer);
			
			if(pointer + size > memory->exact_size)
				return -EINVAL;
			
			if(pointer_out)
				*pointer_out = (char *)memory->ptr + pointer;
			if(size_out)
				*size_out    = size;
			return 0;
		case ALLOC_FREED:
			break;
	};
	return -EFAULT;
} // }}}
size_t   memory_read  (memory_t *memory, off_t offset, void *buffer, size_t buffer_size){ // {{{
	void                  *memory_ptr;
	uintmax_t              memory_size; 
	
	if(memory_translate(memory, offset, buffer_size, &memory_ptr, &memory_size) != 0)
		return 0;
	
	memcpy(buffer, memory_ptr, memory_size);
	return memory_size;
} // }}}
size_t   memory_write (memory_t *memory, off_t offset, void *buffer, size_t buffer_size){ // {{{
	void                  *memory_ptr;
	uintmax_t              memory_size; 
	
	if(memory_translate(memory, offset, buffer_size, &memory_ptr, &memory_size) != 0)
		return 0;
	
	memcpy(memory_ptr, buffer, memory_size);
	return memory_size;
} // }}}

uintmax_t  data_memory_t_len(data_t *data, data_ctx_t *ctx){
	(void)ctx; // TODO add ctx parse
	return ((memory_t *)data->data_ptr)->exact_size;
}

ssize_t   data_memory_t_read(data_t *data, data_ctx_t *ctx, off_t coffset, void **buffer, uintmax_t *buffer_size){
	ssize_t                ret;
	off_t                  offset            = 0;
	size_t                 size              = __MAX(size_t);
	
	hash_data_copy(ret, TYPE_OFFT,  offset, ctx, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, size,   ctx, HK(size));
	
	offset += coffset;
	
	if(memory_translate(((memory_t *)data->data_ptr), offset, (uintmax_t)size, buffer, buffer_size) != 0)
		return -1;
	
	return *buffer_size;
}

ssize_t   data_memory_t_write(data_t *data, data_ctx_t *ctx, off_t coffset, void *buffer, uintmax_t buffer_size){
	ssize_t                ret;
	off_t                  offset            = 0;
	size_t                 size              = __MAX(size_t);
	uintmax_t              memory_size; 
	
	hash_data_copy(ret, TYPE_OFFT,  offset, ctx, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, size,   ctx, HK(size));
	
	offset      += coffset;
	memory_size  = MIN(size, buffer_size);
	
	if( (ret = memory_write( ((memory_t *)data->data_ptr), offset, buffer, memory_size)) == 0)
		return -EFAULT;
	
	return ret;
}


data_proto_t memory_t_proto = {
	.type          = TYPE_MEMORYT,
	.type_str      = "memory_t",
	.size_type     = SIZE_VARIABLE,
	.func_len      = &data_memory_t_len,
	.func_rawlen   = &data_memory_t_len,
	.func_read     = &data_memory_t_read,
	.func_write    = &data_memory_t_write
};

