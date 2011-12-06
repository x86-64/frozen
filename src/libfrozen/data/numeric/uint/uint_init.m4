m4_define(`NAME', m4_substr(NAME(), `0', m4_index(NAME(), `.')))
m4_define(`DEF', m4_translit(NAME(), `a-z', `A-Z'))
m4_define(`DEF', m4_patsubst(DEF(), `_', `'))
m4_define(`TYPE', NAME())
m4_ifelse(NAME(), `int_t',    `m4_define(`TYPE', `intmax_t') ')
m4_ifelse(NAME(), `uint_t',   `m4_define(`TYPE', `uintmax_t')')
m4_changequote([,])

