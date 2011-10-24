#ifndef DATA_SLICE_T
#define DATA_SLICE_T

#define DATA_SLICET(_data,_off,_size)  {TYPE_SLICET, (slice_t []){ { _data, _off, _size } }}
#define DEREF_TYPE_SLICET(_data) (slice_t *)((_data)->ptr)
#define REF_TYPE_SLICET(_dt) _dt
#define HAVEBUFF_TYPE_SLICET 0

typedef struct slice_t {
	data_t                *data;
	uintmax_t              off;
	uintmax_t              size;
} slice_t;

#endif
