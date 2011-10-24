#ifndef DATA_IO_T
#define DATA_IO_T

#define DATA_IOT(_ud, _func)  {TYPE_IOT, (io_t []){ { _ud, _func } }}
#define DEREF_TYPE_IOT(_data) (io_t *)((_data)->ptr)
#define REF_TYPE_IOT(_dt) _dt
#define HAVEBUFF_TYPE_IOT 0

typedef ssize_t (*f_io_func) (void *userdata, void *args);

typedef struct io_t {
	void *                 ud;
	f_io_func              handler;
} io_t;

#endif
