#ifndef DATA_IO_T
#define DATA_IO_T

#define DATA_IOT(_fread, _fwrite, _ud)  {TYPE_IOT, (io_t []){ { _fread, _fwrite, _ud } }}

typedef struct io_t {
	f_data_func            read;
	f_data_func            write;
	data_t                 ud;
} io_t;

extern data_proto_t io_t_proto;

#endif
