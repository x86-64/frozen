#ifndef DATA_IO_T
#define DATA_IO_T

#define DATA_IOT(_fread, _fwrite)  {TYPE_IOT, (io_t []){ { _fread, _fwrite } }, 0}

typedef struct io_t {
	f_data_read      read;
	f_data_write     write;
} io_t;

extern data_proto_t io_t_proto;

#endif
