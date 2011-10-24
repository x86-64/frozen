m4_include(uint_init.m4)

[#ifndef DATA_UINT_]DEF()_H
[#define DATA_UINT_]DEF()_H

[#define DATA_]DEF()[(value) { TYPE_]DEF()[, (]TYPE()[ []){ value } } ]
[#define DATA_PTR_]DEF()[(value) { TYPE_]DEF()[, value } ]
[#define DEREF_TYPE_]DEF()[(_data) *(]TYPE()[ *)((_data)->ptr) ]
[#define REF_TYPE_]DEF()[(_dt) (&(_dt)) ]
[#define HAVEBUFF_TYPE_]DEF() 1

[#endif]
/* vim: set filetype=m4: */
