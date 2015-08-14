build = $(top_builddir)/src/util/oxim-conv/oxim2tab

tablesdir = $(TABLES_DIR)

tables_DATA = cnscj.tab \
	      cnsphone.tab

all: $(tables_DATA)

cnscj.tab: cnscj.cin
	$(build) -o $@ cnscj.cin
cnsphone.tab: cnsphone.cin
	$(build) -o $@ cnsphone.cin

install-data-local:
	$(mkinstalldirs) $(DESTDIR)/$(tablesdir)
	$(INSTALL_DATA) default.phr $(DESTDIR)/$(tablesdir)
	$(INSTALL_DATA) symbol.list $(DESTDIR)/$(tablesdir)

uninstall-local:
	rm -f $(DESTDIR)/$(tablesdir)/default.phr
	rm -f $(DESTDIR)/$(tablesdir)/symbol.list

CLEANFILES = $(tables_DATA)
