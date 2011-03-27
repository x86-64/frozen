#ifndef DATA_BUFFER_T_H
#define DATA_BUFFER_T_H

enum { TYPE_BUFFERT = 22 };
#define DATA_BUFFERT(_buff)   { TYPE_BUFFERT, (void *)_buff, 0 }

extern data_proto_t buffer_t_proto;

#endif
