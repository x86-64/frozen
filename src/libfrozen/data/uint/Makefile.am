uint_src=int8_t.c int16_t.c int32_t.c int64_t.c uint8_t.c uint16_t.c uint32_t.c uint64_t.c uint_t.c int_t.c off_t.c size_t.c
uint_hdr=int8_t.h int16_t.h int32_t.h int64_t.h uint8_t.h uint16_t.h uint32_t.h uint64_t.h uint_t.h int_t.h off_t.h size_t.h

noinst_LTLIBRARIES                   = libfrozen_data_uint.la
libfrozen_data_uint_la_CFLAGS        = -DDATA_TYPE_SOURCE -I@top_srcdir@/src/libfrozen/core/ -I@top_srcdir@/src/libfrozen/data/
libfrozen_data_uint_la_SOURCES       = 
EXTRA_libfrozen_data_uint_la_SOURCES = $(uint_src) $(uint_hdr) uint_init.m4
libfrozen_data_uint_la_LIBADD        = @DATA_OBJECTS_uint@
libfrozen_data_uint_la_DEPENDENCIES  = @DATA_OBJECTS_uint@

EXTRA_DIST = uint.c.m4 uint.h.m4

$(uint_src): $(uint_hdr) uint.c.m4
	${M4} -sP -DNAME=$@ uint.c.m4 > $@
$(uint_hdr): uint.h.m4
	${M4} -sP -DNAME=$@ $< > $@

CLEANFILES = $(uint_src) $(uint_hdr)
