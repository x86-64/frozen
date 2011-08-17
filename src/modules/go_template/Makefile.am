MYLDFLAGS= -lgofrozen -lgofrozen_wrap -L@top_srcdir@/src/lang/gogcc/

EXTRA_DIST = main.go
CLEANFILES = module_go_template.so
modules_DATA = module_go_template.so

module_go_template.so: main.go
	$(GCCGO) -shared -fPIC $(CFLAGS) $(MYLDFLAGS) -o $@ $< 
