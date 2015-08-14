Summary: Open X Input Method Server.
Name: oxim
Version: 1.2.1
Release: 3%{?dist}
License: GPL
Group: System/Internationalization
Source0: %{name}-%{version}.tar.gz
Autoreq: no
Requires: zlib, libchewing >= 0.2.6, libXpm >= 2.0.0, libXtst >= 1.0.0, libXft >= 2.0
Requires(post,preun): /usr/sbin/alternatives
BuildRequires: zlib-devel, libchewing-devel >= 0.2.6, libXpm-devel >= 2.0.0, libXtst-devel >= 1.0.0, libXft-devel >= 2.0
%if 0%{?fedora} >= 10
BuildRequires: qt3-devel >= 3.2.2
%else
BuildRequires: qt-devel >= 3.2.2
%endif
Buildroot: %{_tmppath}/%{name}-buildroot

Obsoletes: xcin >= 3.0.0, oxim-qt-immodule, oxim-chewing-module, oxim-gtk-immodule, oxim-unicode-module

%description
An X Input Method Server.
gtk-immodule : OXIM context plugin for Gtk input method module.
qt-immodule : OXIM context plugin for QT input method module.
unicode-module : Unicode input method.
chewing-module : Chewing input method.

%prep

rm -rf $RPM_BUILD_ROOT

%setup -q

%build
unset QTDIR && . /etc/profile.d/qt.sh
%define qt_dir %(echo $QTDIR)

%configure --enable-static=no \
	--with-qt-dir=${QTDIR}

%{__make} %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
DESTDIR=$RPM_BUILD_ROOT make install-strip
find $RPM_BUILD_ROOT -type f -name "*.la" -exec rm -f {} \;

%post
/sbin/ldconfig > /dev/null 2>&1
alternatives --install %{_sysconfdir}/X11/xinit/xinputrc xinputrc %{_sysconfdir}/X11/xinit/xinput.d/oxim 99

%preun
if [ "$1" = 0 ]; then
   alternatives --remove xinputrc %{_sysconfdir}/X11/xinit/xinput.d/oxim
fi

%postun
/sbin/ldconfig > /dev/null 2>&1

%clean 
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc {README,COPYING,AUTHORS}
%dir %{_sysconfdir}/oxim
%config %{_sysconfdir}/oxim/*
%{_bindir}/oxim-conv
%{_bindir}/oxim2tab
%{_bindir}/oxim
%{_libdir}/liboxim.so*
%dir %{_libdir}/oxim
%dir %{_libdir}/oxim/modules
%{_libdir}/oxim/modules/gen-inp.so
%{_libdir}/oxim/modules/gen-inp-v1.so
%dir %{_libdir}/oxim/tables
%{_libdir}/oxim/tables/cnscj.tab
%{_libdir}/oxim/tables/cnsphone.tab
%{_libdir}/oxim/tables/default.phr
%{_libdir}/oxim/tables/symbol.list
%dir %{_libdir}/oxim/panels
%{_libdir}/oxim/panels/defaultkeyboard.*
%{_sysconfdir}/X11/xinit/xinput.d/oxim
%dir %{_libdir}/oxim/immodules

%{_libdir}/oxim/immodules/gtk-im-oxim.so
%{_libdir}/gtk-2.0/immodules/gtk-im-oxim.so

%dir %{_libdir}/oxim/immodules
%{_libdir}/oxim/immodules/qt-im-oxim.so
%{qt_dir}/plugins/inputmethods/qt-im-oxim.so

%{_libdir}/oxim/modules/unicode.so
%{_libdir}/oxim/modules/chewing.so

%{_mandir}/man1/oxim.1.gz

#%exclude %{_datadir}/gettext
%{_datadir}/locale/*/LC_MESSAGES/oxim.mo


%changelog
* Thu Nov 27 2008 Wind Win <yc.yan@ossii.com.tw> 1.2.0-4
- Build for most fixes.

* Thu Nov 6 2008 Wind Win <yc.yan@ossii.com.tw> 1.2.0-3
- Rebuild for CVS update.

* Mon Oct 27 2008 Wind Win <yc.yan@ossii.com.tw> 1.2.0-2
- Modify for support I18N of oxim tables' name.

* Mon Oct 14 2008 Wind Win <yc.yan@ossii.com.tw> 1.2.0-1
- Add I18N - zh_TW & zh_CN gettext mo&po files. 
- Add BuildRequire packages.(libXpm, libXft, libXtst)

* Tue Sep 16 2008 Wind Win <yc.yan@ossii.com.tw> 1.1.6-2
- Add man page file - oxim.1

* Fri Sep 5 2008 Wind Win <yc.yan@ossii.com.tw> 1.1.6-1
- Initial RPM build.
