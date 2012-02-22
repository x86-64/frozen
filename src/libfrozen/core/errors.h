#define ESTEP 4096
#define ECOMMON 200
#define ECOUNTER __LINE__
#define DYNAMIC_EMODULE_START 200

#ifdef FROZEN_MODULE
	uintmax_t       emodule;
	#define EMODULE_STEP emodule
#else
	#define EMODULE_STEP ERRORS_MODULE_ID * ESTEP
#endif

#define error(...)   -(EMODULE_STEP + ECOUNTER + ECOMMON)
#define errorn(err)  -(EMODULE_STEP + err)

typedef void (*log_func)(char *format, va_list args);

typedef struct err_item {
        intmax_t    errnum;
        const char *errmsg;
} err_item;

API const char *       errors_describe               (intmax_t errnum);
API void               errors_log                    (char *format, ...);
API void               errors_set_log_func           (log_func func);
API void               errors_register               (err_item *err_list, uintmax_t *emodule);
API ssize_t            errors_is_unix                (intmax_t errnum, intmax_t unix_err);
