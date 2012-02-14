#define EBASE 4096
#define ESTEP 4096
#define ECOUNTER __LINE__
#define DYNAMIC_EMODULE_START 200

#ifdef FROZEN_MODULE
	uintmax_t       emodule;
	#define EMODULE emodule
#endif

#define error(...)   -(EBASE + EMODULE * ESTEP + ECOUNTER)

typedef void (*log_func)(char *format, va_list args);

typedef struct err_item {
        intmax_t    errnum;
        const char *errmsg;
} err_item;

API const char *       describe_error                (intmax_t errnum);
API void               log_error                     (char *format, ...);
API void               set_log_func                  (log_func func);
API void               errors_register               (err_item *err_list, uintmax_t *emodule);

