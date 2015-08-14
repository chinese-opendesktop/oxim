AUTOMAKE_OPTIONS = gnu

input_d_dir = /etc/X11/xinit/xinput.d
xsession_d_dir = /etc/X11/Xsession.d
epc_autostart_dir = $(datadir)/autostart

SUBDIRS = \
	  po \
	  src/lib \
	  src/gtk-immodule \
	  src/gtk-immodule3 \
	  src/qt-immodule \
	  src/qt4-immodule-test \
	  src/modules/gen-inp \
	  src/modules/gen-inp-v1 \
	  src/modules/unicode \
	  src/modules/chewing \
	  src/modules/sunpinyin \
	  src/util/oxim-conv \
	  src/xim/IMdkit \
	  src/xim \
	  tables \
	  panels \
	  etc
#	  src/qt4-immodule \
EXTRA_DIST = \
        oxzilla.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = oxim.pc

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(input_d_dir)
	sed 's|@OXIM_IM_MODULE_DIR@|$(IM_MODULES_DIR)|' oxim-start.sh.in > $(DESTDIR)/$(input_d_dir)/oxim.conf
	$(mkinstalldirs) $(DESTDIR)/$(mandir)/man1/
	$(INSTALL_DATA) oxim.1 $(DESTDIR)/$(mandir)/man1/oxim.1
	$(mkinstalldirs) $(DESTDIR)/$(includedir)/oxim
	$(INSTALL_DATA) src/include/*.h $(DESTDIR)/$(includedir)/oxim/
if ENABLE_EEEPC
	$(mkinstalldirs) $(DESTDIR)/$(xsession_d_dir)
	ln -fs $(input_d_dir)/oxim $(DESTDIR)/$(xsession_d_dir)/90EeePC-oxim
	$(mkinstalldirs) $(DESTDIR)/$(epc_autostart_dir)
	$(INSTALL_DATA) autostart-oxim.desktop $(DESTDIR)/$(epc_autostart_dir)/oxim.desktop
endif

uninstall-local:
	rm -f $(DESTDIR)/$(input_d_dir)/oxim
if ENABLE_EEEPC
	rm -f $(DESTDIR)/$(xsession_d_dir)/90EeePC-oxim
	rm -f $(DESTDIR)/$(epc_autostart_dir)/oxim.desktop
endif
