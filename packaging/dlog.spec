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

%package -n dlog-tests
Summary:    unit testing dlog
Group:      Development/Libraries
Requires:   lib%{name} = %{?epoch:%{epoch}:}%{version}-%{release}

%description -n dlog-tests
Unit testing dlog

%prep
%setup -q

%build
cp %{SOURCE101} .
cp %{SOURCE102} .
%autogen --disable-static
%configure --disable-static \
			--enable-fatal_on \
			--enable-engineer_mode \
			--enable-debug_enable \
			--with-systemd-journal=no

make %{?jobs:-j%jobs} \
	CFLAGS+=-DKMSG_DEV_CONFIG_FILE=\\\"%{_localstatedir}/dlog/dlog_init.conf\\\"
make -C unittests -f Makefile.tests

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/scripts/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/
install -m 0644 %SOURCE301 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE302 %{buildroot}%{_libdir}/systemd/system/

ln -s ../dlog_logger.path %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/dlog_logger.path

install -m 0644 %SOURCE401 %{buildroot}%{_unitdir}

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/%{name}
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/libdlog
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/dlogutil

mkdir -p %{buildroot}/opt/etc
cp %SOURCE201 %{buildroot}/opt/etc/dlog.conf

# default set log output to external files
cp %SOURCE202 %{buildroot}/opt/etc/dlog_logger.conf

mkdir -p %{buildroot}%{_localstatedir}/dlog

mkdir -p %{buildroot}/var/log/dlog

install -m 0755 %{_builddir}/%{name}-%{version}/unittests/dlogtest %{buildroot}/usr/bin/

%preun
systemctl disable dlog.service

%post
systemctl enable dlog.service

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
%attr(-,root,root) %{_unitdir}/dlog.service
%dir %attr(755,root,root) %{_localstatedir}/dlog

%files  -n dlogutil
%manifest dlogutil.manifest
/usr/share/license/dlogutil
%attr(750,log,log) %{_bindir}/dlog_logger
%attr(750,log,log) %{_bindir}/dlogutil
%attr(755,log,log) %{_bindir}/dlogctrl
%attr(664,log,log) /opt/etc/dlog_logger.conf
%{_libdir}/systemd/system/dlog_logger.service
%{_libdir}/systemd/system/dlog_logger.path
%{_libdir}/systemd/system/multi-user.target.wants/dlog_logger.path
%attr(755,log,log) /var/log/dlog

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

%files -n dlog-tests
/usr/bin/dlogtest
