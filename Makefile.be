# Makefile for XaoS on BeOS

# Default location for applications on BeOS.
# See also definition of DATAPATH in aconfig.be
appsdir=/boot/apps
xaosdir=$(appsdir)/XaoS
infodir=$(xaosdir)/doc
mandir=$(xaosdir)/man

INSTALL=install
MIMESET=mimeset

default all clean realclean echo help depend xaos:
	@$(MAKE) -C src -f Makefile.be $@

distclean:
	@$(MAKE) -C src -f Makefile.be distclean
	rm -f src/include/config.h src/include/aconfig.h

install: 
	$(INSTALL) -d $(xaosdir)
	$(INSTALL) -d $(xaosdir)/tutorial
	$(INSTALL) -d $(xaosdir)/examples
	$(INSTALL) -d $(xaosdir)/catalogs
	$(INSTALL) -d $(xaosdir)/doc
	$(INSTALL) -d $(mandir)
	$(INSTALL) -d $(mandir)/man6
	$(INSTALL) -d $(infodir)
	$(INSTALL) bin/xaos $(xaosdir)
	$(INSTALL) -m 444 tutorial/*.x[ah]f $(xaosdir)/tutorial
	$(INSTALL) -m 444 examples/* $(xaosdir)/examples
	$(INSTALL) -m 444 catalogs/* $(xaosdir)/catalogs
	$(INSTALL) -m 444 doc/README doc/README.bugs doc/compilers.txt doc/ANNOUNCE doc/PROBLEMS doc/tutorial.txt $(xaosdir)/doc
	$(INSTALL) -m 444 doc/xaos.6 $(mandir)/man6
	$(INSTALL) -m 444 doc/xaos.info $(infodir)
	# Update MIME types - $(INSTALL) loses them.
	$(MIMESET) $(xaosdir)
	$(MIMESET) $(infodir)
	$(MIMESET) $(mandir)
