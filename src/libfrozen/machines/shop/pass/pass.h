#ifndef PASS_C_H
#define PASS_C_H

ssize_t            stack_call(machine_t *return_to);
ssize_t            stack_clean(void);
machine_t *        stack_return(void);

#endif
