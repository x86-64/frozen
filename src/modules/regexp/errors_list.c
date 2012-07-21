
static err_item errs_list[] = {
 { -408, "src/modules/regexp/regexp.c: can not convert data to string" },
 { -403, "src/modules/regexp/regexp.c: no input string in request" },
 { -305, "src/modules/regexp/regexp.c: calloc failed" },
 { -279, "src/modules/regexp/regexp.c: invalid regexp supplied - compilation error" },
 { -12, "src/modules/regexp/regexp.c: ENOMEM in module " },

};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
