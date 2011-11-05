PERLMAKEFILE=perl/Makefile
PERLOUTDIR=perl/
GODIR=go
GOGCCDIR=gogcc

$(PERLMAKEFILE):
	swig -I$(PERLOUTDIR) -perl5 -outdir $(PERLOUTDIR) -o $(PERLOUTDIR)swig_wrap.c @HAVE_DEBUG@ swig.i
	cd $(PERLOUTDIR) && $(PERL) Makefile.PL

all-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(MAKE) $(AM_MAKEFLAGS)

clean-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(MAKE) $(AM_MAKEFLAGS) clean
	rm -rf $(PERLOUTDIR)/Frozen.pm $(PERLOUTDIR)/swig_wrap.c $(PERLOUTDIR)/Makefile.old $(PERLOUTDIR)/data_*.dat

check-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(PERL) t/Frozen.t

install-exec-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(MAKE) $(AM_MAKEFLAGS) DESTDIR=$(DESTDIR) install

CLEANFILES = 
EXTRA_DIST = \
	swig.i hashkeys.i \
	$(PERLOUTDIR)/Frozen.pm $(PERLOUTDIR)/swig_wrap.c $(PERLOUTDIR)/Makefile.PL $(PERLOUTDIR)/t/*.t \
	$(GODIR)/frozen.swig $(GODIR)/hashkeys.i \
	$(GOGCCDIR)/frozen.swig $(GOGCCDIR)/hashkeys.i

if HAVE_GCCGO
OPTIONAL = gogcc
endif

SUBDIRS = . go $(OPTIONAL)
