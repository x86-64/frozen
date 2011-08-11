MYLDFLAGS= -lgofrozen -lgofrozen_wrap -L@top_srcdir@/src/lang/gogcc/

EXTRA_DIST = main.go
CLEANFILES = module_go_http.so
modules_DATA = module_go_http.so

module_go_http.so: main.go
	$(GCCGO) -shared -fPIC $(CFLAGS) $(MYLDFLAGS) -o $@ $< 
