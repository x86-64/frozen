#define EBASE         4096
#define ESTEP         4096

#define EFLAG_DEBUG   1
#define EFLAG_NOTICE  2
#define EFLAG_WARNING 3
#define EFLAG_ERROR   4

#ifndef __COUNTER__
#error Counter macro not defined       
// TODO handle this (stick to __LINE__?)
#else
#define ECOUNTER __COUNTER__
#endif

#define debug(...)   handle_error(EFLAG_DEBUG,   -(EBASE + EMODULE * ESTEP + ECOUNTER))
#define notice(...)  handle_error(EFLAG_NOTICE,  -(EBASE + EMODULE * ESTEP + ECOUNTER))
#define warning(...) handle_error(EFLAG_WARNING, -(EBASE + EMODULE * ESTEP + ECOUNTER))
#define error(...)   handle_error(EFLAG_ERROR,   -(EBASE + EMODULE * ESTEP + ECOUNTER))

    intmax_t           handle_error             (uintmax_t eflag, intmax_t errnum);
API const char *       describe_error           (intmax_t errnum);

