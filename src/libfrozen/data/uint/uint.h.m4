m4_include(uint_init.m4)

[#ifndef DATA_UINT_]DEF()_H
[#define DATA_UINT_]DEF()_H

[#define DATA_]DEF()[(value) { TYPE_]DEF()[, (]TYPE()[ []){ value } } ]
[#define DATA_PTR_]DEF()[(value) { TYPE_]DEF()[, value } ]

extern data_proto_t NAME()_proto;

[#endif]
/* vim: set filetype=m4: */
