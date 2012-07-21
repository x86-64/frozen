
static err_item errs_list[] = {
 { -246, "src/modules/pcre/pcre.c: regexp compile failed" },
 { -235, "src/modules/pcre/pcre.c: regexp not supplied" },
 { -231, "src/modules/pcre/pcre.c: calloc returns null" },
 { -38, "src/modules/pcre/pcre.c: ENOSYS in module " },

};
#define            errs_list_size      sizeof(errs_list[0])
#define            errs_list_nelements sizeof(errs_list) / errs_list_size
