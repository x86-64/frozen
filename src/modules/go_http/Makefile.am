MYLDFLAGS= -lgofrozen -lgofrozen_wrap -L@top_srcdir@/src/lang/gogcc/

if HAVE_SWIG
if HAVE_GCCGO
MODULES = module_go_http.so
endif
endif

modules_DATA = $(MODULES)
CLEANFILES = $(MODULES)
EXTRA_DIST = main.go

$(MODULES): main.go
	$(GCCGO) -shared -fPIC $(CFLAGS) $(MYLDFLAGS) -o $@ $< 
