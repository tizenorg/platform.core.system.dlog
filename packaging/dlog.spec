Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    15
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  packaging/dlogutil.manifest
Source102:  packaging/libdlog.manifest
Source201:  packaging/dlog.conf.in
Source202:  packaging/dlog_logger.conf.in
Source203:  packaging/dlog_logger.conf-micro.in
Source204:  packaging/dlog_logger.conf-debug.in
Source301:  packaging/dlog_logger.service
Source302:  packaging/dlog_logger.path
Source401:  packaging/dlog.service

%define systemd_journal ON

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: pkgconfig(libsystemd-journal)
BuildRequires: pkgconfig(capi-base-common)
BuildRequires: pkgconfig(libudev)
Requires(post): coreutils
Requires(post): /usr/bin/systemctl
Requires(postun): /usr/bin/systemctl
Requires(preun): /usr/bin/systemctl

%description
dlog API library

%package -n libdlog
Summary:    Logging service dlog API
Group:      Development/Libraries

%description -n libdlog
dlog API library

%package -n libdlog-devel
Summary:    Logging service dlog API
Group:      Development/Libraries
Requires:   lib%{name} = %{?epoch:%{epoch}:}%{version}-%{release}

%description -n libdlog-devel
dlog API library


%package -n dlogutil
Summary:    print log data to the screen
Group:      Development/Libraries
Requires:   lib%{name} = %{?epoch:%{epoch}:}%{version}-%{release}
Requires(post): /usr/bin/systemctl
Requires(postun): /usr/bin/systemctl
Requires(preun): /usr/bin/systemctl

%description -n dlogutil
Utilities for print log data

%if %{?systemd_journal} == OFF
%package -n dlog-tests
Summary:    unit testing dlog
Group:      Development/Libraries
Requires:   lib%{name} = %{?epoch:%{epoch}:}%{version}-%{release}

%description -n dlog-tests
Unit testing dlog
%endif

%prep
%setup -q

%build
cp %{SOURCE101} .
cp %{SOURCE102} .
%autogen --disable-static
%configure --disable-static \
			--enable-fatal_on \
%if %{?systemd_journal} == ON
			--enable-journal \
%endif
			--enable-engineer_mode
make %{?jobs:-j%jobs} \
	CFLAGS+=-DKMSG_DEV_CONFIG_FILE=\\\"%{_localstatedir}/dlog/dlog_init.conf\\\"
%if %{?systemd_journal} == OFF
make -C unittests -f Makefile.tests
%endif

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/scripts/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}/opt/etc
cp %SOURCE201 %{buildroot}/opt/etc/dlog.conf


%if %{?systemd_journal} == OFF
mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants/
install -m 0644 %SOURCE301 %{buildroot}%{_unitdir}
install -m 0644 %SOURCE302 %{buildroot}%{_unitdir}
ln -s ../dlog_logger.path %{buildroot}%{_unitdir}/multi-user.target.wants/dlog_logger.path

# default set log output to external files
cp %SOURCE202 %{buildroot}/opt/etc/dlog_logger.conf
install -m 0644 %SOURCE401 %{buildroot}%{_unitdir}
%endif

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/%{name}
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/libdlog
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/dlogutil

mkdir -p %{buildroot}/var/log/dlog

%if %{?systemd_journal} == ON
# Workaround: replace with dlogutil script due to scheduling issue
rm %{buildroot}/usr/bin/dlogutil
cp %{_builddir}/%{name}-%{version}/scripts/dlogutil.sh %{buildroot}/usr/bin/dlogutil
%else
install -m 0755 %{_builddir}/%{name}-%{version}/unittests/dlogtest %{buildroot}/usr/bin/
%endif

mkdir -p %{buildroot}%{_localstatedir}/dlog
mkdir -p %{buildroot}/var/log/dlog

%preun
systemctl disable dlog.service || true

%post
systemctl enable dlog.service || true

%postun
systemctl daemon-reload

%preun -n dlogutil
systemctl disable dlog_logger.service

%post -n dlogutil
systemctl enable dlog_logger.service

%postun -n dlogutil
systemctl daemon-reload

%post -n libdlog
/sbin/ldconfig

%postun -n libdlog
/sbin/ldconfig

%files
/usr/share/license/%{name}
%attr(700,root,root) %{_sbindir}/dloginit
%dir %attr(755,root,root) %{_localstatedir}/dlog
%if %{?systemd_journal} == OFF
%attr(-,root,root) %{_unitdir}/dlog.service
%endif

%files  -n dlogutil
%manifest dlogutil.manifest
/usr/share/license/dlogutil
%attr(750,log,log) %{_bindir}/dlogutil
%attr(755,log,log) %{_bindir}/dlogctrl
%attr(755,log,log) /var/log/dlog
%if %{?systemd_journal} == OFF
%attr(750,log,log) %{_bindir}/dlog_logger
%attr(664,log,log) /opt/etc/dlog_logger.conf
%{_unitdir}/dlog_logger.service
%{_unitdir}/dlog_logger.path
%{_unitdir}/multi-user.target.wants/dlog_logger.path
%endif

%files  -n libdlog
%manifest libdlog.manifest
/usr/share/license/libdlog
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0
%attr(664,log,log) /opt/etc/dlog.conf

%files -n libdlog-devel
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

%if %{?systemd_journal} == OFF
%files -n dlog-tests
/usr/bin/dlogtest
%endif
