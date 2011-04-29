PERLMAKEFILE=perl/Makefile
PERLOUTDIR=perl/

$(PERLMAKEFILE):
	swig -perl5 -outdir $(PERLOUTDIR) -o $(PERLOUTDIR)swig_wrap.c @HAVE_DEBUG@ swig.i
	cd $(PERLOUTDIR) && $(PERL) Makefile.PL

all-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(MAKE) $(AM_MAKEFLAGS)

clean-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(MAKE) $(AM_MAKEFLAGS) clean
	rm -rf $(PERLOUTDIR)/Makefile.old
	rm -rf $(PERLOUTDIR)/data_*.dat

check-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(PERL) t/Frozen.t

install-exec-local: $(PERLMAKEFILE)
	cd $(PERLOUTDIR) && $(MAKE) $(AM_MAKEFLAGS) DESTDIR=$(DESTDIR) install

EXTRA_DIST = swig.i $(PERLOUTDIR)/Frozen.pm $(PERLOUTDIR)/swig_wrap.c $(PERLOUTDIR)/Makefile.PL $(PERLOUTDIR)/t/*.t
