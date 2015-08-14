
confdir = $(OXIM_DEFAULT_RCDIR)

conf_DATA = locale.list

install-exec-local:
	mkdir -p $(DESTDIR)/$(OXIM_DEFAULT_RCDIR)
if ENABLE_EEEPC
	$(INSTALL_DATA) oxim.conf.eeepc $(DESTDIR)/$(OXIM_DEFAULT_RCDIR)/oxim.conf
else
	$(INSTALL_DATA) oxim.conf.default $(DESTDIR)/$(OXIM_DEFAULT_RCDIR)/oxim.conf
endif
if ENABLE_DEVICE
	$(INSTALL_DATA) panel.conf.device $(DESTDIR)/$(OXIM_DEFAULT_RCDIR)/panel.conf
else
	$(INSTALL_DATA) panel.conf.default $(DESTDIR)/$(OXIM_DEFAULT_RCDIR)/panel.conf
endif

uninstall-local:
	rm -f $(DESTDIR)/$(OXIM_DEFAULT_RCDIR)/oxim.conf
