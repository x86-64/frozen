MYLDFLAGS= -lgofrozen -lgofrozen_wrap -L@top_srcdir@/src/lang/gogcc/

EXTRA_DIST = test.go
CLEANFILES = module_test.so
modules_DATA = module_test.so

module_test.so: test.go
	$(GCCGO) -shared -fPIC $(CFLAGS) $(MYLDFLAGS) -o $@ $< 
