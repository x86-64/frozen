#ifndef DATA_SLIDER_T
#define DATA_SLIDER_T

#define DATA_SLIDERT(_data,_off)  {TYPE_SLIDERT, (slider_t []){ { _data, _off } }}
#define DEREF_TYPE_SLIDERT(_data) (slider_t *)((_data)->ptr)
#define REF_TYPE_SLIDERT(_dt) _dt
#define HAVEBUFF_TYPE_SLIDERT 0

typedef struct slider_t {
	data_t                *data;
	uintmax_t              off;
} slider_t;

#endif
