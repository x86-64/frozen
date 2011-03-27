m4_include(uint_init.m4)

[#ifndef DATA_UINT_]DEF()_H
[#define DATA_UINT_]DEF()_H

enum {
 [TYPE_]DEF() = DEF_NUM()
};

[#define DATA_]DEF()[(value) { TYPE_]DEF()[, (]TYPE()[ []){ value }, sizeof(]TYPE()[) } ]
[#define DATA_PTR_]DEF()[(value) { TYPE_]DEF()[, value, sizeof(]TYPE()[) } ]
[#define DT_]DEF() TYPE()
[#define GET_TYPE_]DEF()[(value) *((]TYPE()[ *)(value)->data_ptr)]

extern data_proto_t NAME()_proto;

[#endif]
/* vim: set filetype=m4: */
