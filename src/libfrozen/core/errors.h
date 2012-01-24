#define EBASE 4096
#define ESTEP 4096
#define ECOUNTER __LINE__

#define EFLAG_DEBUG   1
#define EFLAG_NOTICE  2
#define EFLAG_WARNING 3
#define EFLAG_ERROR   4

#define debug(...)   handle_error(EFLAG_DEBUG,   -(EBASE + EMODULE * ESTEP + ECOUNTER))
#define notice(...)  handle_error(EFLAG_NOTICE,  -(EBASE + EMODULE * ESTEP + ECOUNTER))
#define warning(...) handle_error(EFLAG_WARNING, -(EBASE + EMODULE * ESTEP + ECOUNTER))
#define error(...)   handle_error(EFLAG_ERROR,   -(EBASE + EMODULE * ESTEP + ECOUNTER))

typedef void (*log_func)(char *format, va_list args);

    intmax_t           handle_error             (uintmax_t eflag, intmax_t errnum);
API const char *       describe_error           (intmax_t errnum);
API void               log_error                (char *format, ...);
API void               set_log_func             (log_func func);

