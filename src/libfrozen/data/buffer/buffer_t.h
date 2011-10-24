#ifndef DATA_BUFFER_T_H
#define DATA_BUFFER_T_H

#define DATA_BUFFERT(_buff)   { TYPE_BUFFERT, (void *)_buff }
#define DEREF_TYPE_BUFFERT(_data) (buffer_t *)((_data)->ptr)
#define REF_TYPE_BUFFERT(_dt) _dt
#define HAVEBUFF_TYPE_BUFFERT 0

#endif
