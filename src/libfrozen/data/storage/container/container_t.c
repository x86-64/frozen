#include <libfrozen.h>
#include <container_t.h>

#define container_process(_container,_start_offset,_size,_func)  ({         \
	data_t                *chunk_data;                                  \
	uintmax_t              chunk_offset;                                \
	uintmax_t              chunk_size;                                  \
	uintmax_t              _curr_offset, _to_process, _call;            \
	container_chunk_t     *_chunk            = _container->head;        \
	                                                                    \
	_curr_offset = 0;                                                   \
	_to_process  = _size;                                               \
	while(_to_process > 0){                                             \
		if(_chunk == NULL)                                          \
			break;                                              \
		                                                            \
		_call      = 1;                                             \
		chunk_size = chunk_get_size(_chunk);                        \
		                                                            \
		if(                                                         \
			_start_offset >= _curr_offset               &&      \
			_start_offset <  _curr_offset + chunk_size          \
		){                                                          \
			chunk_offset = (_start_offset - _curr_offset);      \
		}else if(_curr_offset > _start_offset){                     \
			chunk_offset = 0;                                   \
		}else{                                                      \
			_call = 0;                                          \
		}                                                           \
		_curr_offset += chunk_size;                                 \
		if(_call == 1){                                             \
			chunk_data = &_chunk->data;                         \
			chunk_size = chunk_size - chunk_offset;             \
			chunk_size = MIN(chunk_size, _to_process);          \
			_func;                                              \
			                                                    \
			_to_process  -= chunk_size;                         \
		}                                                           \
		(void)chunk_offset; (void)chunk_size; (void)chunk_data;     \
		                                                            \
		_chunk        = _chunk->cnext;                              \
	}                                                                   \
})

static ssize_t data_container_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = container_size( (container_t *)data->ptr );
	return 0;
} // }}}
static ssize_t data_container_t_read(data_t *data, fastcall_read *fargs){ // {{{
	if(data->ptr == NULL)
		return -EINVAL;
	
	return container_read(
		(container_t *)data->ptr,
		fargs->offset,
		fargs->buffer,
		fargs->buffer_size,
		&fargs->buffer_size
	);
} // }}}
static ssize_t data_container_t_write(data_t *data, fastcall_write *fargs){ // {{{
	if(data->ptr == NULL)
		return -EINVAL;
	
	return container_write(
		(container_t *)data->ptr,
		fargs->offset,
		fargs->buffer,
		fargs->buffer_size,
		&fargs->buffer_size
	);
} // }}}
static ssize_t data_container_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr == NULL)
		return -EINVAL;
	
	container_free((container_t *)data->ptr);
	return 0;
} // }}}

data_proto_t container_t_proto = {
	.type                   = TYPE_CONTAINERT,
	.type_str               = "container_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_container_t_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_container_t_len,
		[ACTION_READ]        = (f_data_func)&data_container_t_read,
		[ACTION_WRITE]       = (f_data_func)&data_container_t_write,
		[ACTION_FREE]        = (f_data_func)&data_container_t_free,
	}
};

static container_chunk_t *  chunk_alloc(data_t *data, chunk_flags_t flags){ // {{{
	container_chunk_t     *chunk;
	
	if(data == NULL)
		return NULL;
	
	if( (chunk = malloc(sizeof(container_chunk_t))) == NULL)
		return NULL;
	
	chunk->cnext       = NULL;
	chunk->cached_size = 0;
	chunk->flags       = flags;
	
	if( (flags & CHUNK_ADOPT_DATA) != 0){
		memcpy(&chunk->data, data, sizeof(data_t));
	}else{
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &chunk->data };
		if(data_query(data, &r_copy) < 0){
			free(chunk);
			return NULL;
		}
	}
	return chunk;
} // }}}
static void              chunk_free(container_chunk_t *chunk){ // {{{
	if( (chunk->flags & CHUNK_DONT_FREE) == 0){
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&chunk->data, &r_free);
	}
	
	free(chunk);
} // }}}
static uintmax_t         chunk_get_size(container_chunk_t *chunk){ // {{{
	register uintmax_t nc;
	
	nc = ((chunk->flags & CHUNK_CACHE_SIZE) != 0);
	if(nc){
		if( (chunk->flags & INTERNAL_CACHED) != 0)
			return chunk->cached_size;
	}
	
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if(data_query(&chunk->data, &r_len) < 0)
		return 0;
	
	if(nc){
		chunk->cached_size   = r_len.length;
		chunk->flags        |= INTERNAL_CACHED;
	}
	return r_len.length;
} // }}}
		        
static void              container_add_head_any        (container_t *container, container_chunk_t *chunk){ // {{{
	if(container->head == NULL){
		container->tail = chunk;
	}else{
		chunk->cnext = container->head;
	}
	container->head = chunk;
} // }}}
static void              container_add_tail_any        (container_t *container, container_chunk_t *chunk){ // {{{
	if(container->tail == NULL){
		container->head        = chunk;
	}else{
		container->tail->cnext = chunk;
	}
	container->tail = chunk;
} // }}}

void            container_init                (container_t *container){ // {{{
	container->head  = NULL;
	container->tail  = NULL;
} // }}}
void            container_destroy             (container_t *container){ // {{{
	container_clean(container);
} // }}}

container_t *   container_alloc               (void){ // {{{
	container_t           *container;

	if( (container = malloc(sizeof(container_t))) == NULL)
		return NULL;
	
	container_init(container);
	return container;
} // }}}
void            container_free                (container_t *container){ // {{{
	container_destroy(container);
	free(container);
} // }}}

ssize_t         container_add_head_data       (container_t *container, data_t *data, chunk_flags_t flags){ // {{{
	container_chunk_t     *chunk;
	
	if( (chunk = chunk_alloc(data, flags)) == NULL)
		return -ENOMEM;
	
	container_add_head_any(container, chunk);
	return 0;
} // }}}
ssize_t         container_add_tail_data       (container_t *container, data_t *data, chunk_flags_t flags){ // {{{
	container_chunk_t     *chunk;
	
	if( (chunk = chunk_alloc(data, flags)) == NULL)
		return -ENOMEM;
	
	container_add_tail_any(container, chunk);
	return 0;
} // }}}

void            container_clean               (container_t *container){ // {{{
	container_chunk_t     *chunk;
	container_chunk_t     *chunk_next;
	
	for(chunk = container->head; chunk; chunk = chunk_next){
		chunk_next = chunk->cnext;
		chunk_free(chunk);
	}
	container->head = NULL;
	container->tail = NULL;
} // }}}

uintmax_t       container_size                (container_t *container){ // {{{
	uintmax_t              container_size       = 0;
	
	container_process(
		container,
		0,
		~0,
		container_size += chunk_size
	);
	return container_size;
} // }}}
ssize_t         container_write               (container_t *container, uintmax_t offset, void *buf, uintmax_t buf_size, uintmax_t *pwritten){ // {{{
	ssize_t                ret               = -1; // EOF
	uintmax_t              written           = 0;
	
	container_process(
		container,
		offset,
		buf_size,
		do{
			fastcall_write r_write;
			r_write.header.nargs  = 5;
			r_write.header.action = ACTION_WRITE;
			r_write.offset        = chunk_offset;
			r_write.buffer        = buf + written;
			r_write.buffer_size   = chunk_size;
			if( (ret = data_query(chunk_data, &r_write)) < 0)
				return ret;
			
			written += r_write.buffer_size;
		}while(0)
	);
	if(pwritten)
		*pwritten = written;
	
	return ret;
} // }}}
ssize_t         container_read                (container_t *container, uintmax_t offset, void *buf, uintmax_t buf_size, uintmax_t *pbuf_size){ // {{{
	ssize_t                ret               = -1; // EOF
	uintmax_t              read              = 0;
	
	container_process(
		container,
		offset,
		buf_size,
		do{
			fastcall_read r_read;
			r_read.header.nargs  = 5;
			r_read.header.action = ACTION_READ;
			r_read.offset        = chunk_offset;
			r_read.buffer        = buf + read;
			r_read.buffer_size   = chunk_size;
			if( (ret = data_query(chunk_data, &r_read)) < 0)
				return ret;
			
			read += r_read.buffer_size;
		}while(0)
	);
	if(pbuf_size)
		*pbuf_size = read;
	
	return ret;
} // }}}

