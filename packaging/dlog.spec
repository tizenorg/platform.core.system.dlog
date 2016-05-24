Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    17
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  packaging/dlogutil.manifest
Source102:  packaging/libdlog.manifest
Source301:  packaging/dlog_logger.service
Source302:  packaging/dlog_logger.path.kmsg
Source303:  packaging/dlog_logger.path.logger
Source401:  packaging/dloginit.service
Source501:  packaging/01-dlog.rules.kmsg
Source502:  packaging/01-dlog.rules.logger

# Choose dlog backend log device
# Warning : MUST be only one "ON" in below three switches
%define backend_journal	ON
%define backend_kmsg	OFF
%define backend_logger	OFF

# Do NOT touch switches below
%if "%{?tizen_target_name}" == "TM1" || "%{?tizen_target_name}" == "hawkp"
%define backend_journal	OFF
%define backend_kmsg	ON
%define backend_logger	OFF
%endif

%if "%{?profile}" == "wearable" || "%{?_with_emulator}" == "1"
%define backend_journal	OFF
%define backend_kmsg	OFF
%define backend_logger	ON
%endif

BuildRequires: autoconf
BuildRequires: automake
BuildRequires: libtool
BuildRequires: pkgconfig(libsystemd-journal)
BuildRequires: pkgconfig(capi-base-common)
BuildRequires: pkgconfig(libudev)
BuildRequires: pkgconfig(libtzplatform-config)
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


%prep
%setup -q

%build

cp %{SOURCE101} .
cp %{SOURCE102} .
%autogen --disable-static
%configure --disable-static \
			--enable-fatal_on \
		%if %{?backend_journal} == ON
			--enable-journal \
		%endif
		%if %{?backend_kmsg} == ON
			--enable-kmsg \
		%endif
		%if %{?backend_logger} == ON
			--enable-android-logger \
		%endif
			--enable-debug_mode \
			TZ_SYS_ETC=%{TZ_SYS_ETC}
make %{?jobs:-j%jobs} \
	CFLAGS+=-DTZ_SYS_ETC=\\\"%{TZ_SYS_ETC}\\\" \
%if %{?backend_journal} == ON
	CFLAGS+=-DDLOG_BACKEND_JOURNAL
%endif
%if %{?backend_kmsg} == ON
	CFLAGS+=-DDLOG_BACKEND_KMSG
%endif
%if %{?backend_logger} == ON
	CFLAGS+=-DDLOG_BACKEND_LOGGER
%endif


%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/usr/bin/

mkdir -p %{buildroot}%{TZ_SYS_ETC}

%if %{?backend_journal} == OFF

mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants/
install -m 0644 %SOURCE301 %{buildroot}%{_unitdir}

%if %{?backend_kmsg} == ON
install -m 0644 %SOURCE302 %{buildroot}%{_unitdir}/dlog_logger.path
%else
install -m 0644 %SOURCE303 %{buildroot}%{_unitdir}/dlog_logger.path
%endif

ln -s ../dlog_logger.path %{buildroot}%{_unitdir}/multi-user.target.wants/dlog_logger.path

%endif

%if %{?backend_kmsg} == ON
mkdir -p %{buildroot}%{_unitdir}/sysinit.target.wants/
install -m 0644 %SOURCE401 %{buildroot}%{_unitdir}
ln -s ../dloginit.service %{buildroot}%{_unitdir}/sysinit.target.wants/dloginit.service
%endif

mkdir -p %{buildroot}%{_udevrulesdir}

%if %{?backend_kmsg} == ON
install -m 0644 %SOURCE501 %{buildroot}%{_udevrulesdir}/01-dlog.rules
%endif
%if %{?backend_logger} == ON
install -m 0644 %SOURCE502 %{buildroot}%{_udevrulesdir}/01-dlog.rules
%endif

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/%{name}
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/libdlog
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/dlogutil

mkdir -p %{buildroot}/var/log/dlog

%post
systemctl daemon-reload

%post -n dlogutil
systemctl daemon-reload
chsmack -a System /var/log/dlog

%postun -n dlogutil
systemctl daemon-reload

%post -n libdlog
/sbin/ldconfig

%postun -n libdlog
/sbin/ldconfig

%files  -n dlogutil
%manifest dlogutil.manifest
/usr/share/license/dlogutil
%attr(750,log,log) %{_bindir}/dlogutil
%attr(755,log,log) %{_bindir}/dlogctrl
%attr(755,log,log) /var/log/dlog
%if %{?backend_journal} == OFF
%{_udevrulesdir}/01-dlog.rules
%attr(750,log,log) %{_bindir}/dlog_logger
%{_unitdir}/dlog_logger.service
%{_unitdir}/dlog_logger.path
%{_unitdir}/multi-user.target.wants/dlog_logger.path
%endif

%files  -n libdlog
%manifest libdlog.manifest
/usr/share/license/libdlog
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0
%attr(664,log,log) %{TZ_SYS_ETC}/dlog.conf
/usr/share/license/%{name}
%if %{?backend_kmsg} == ON
%attr(700,log,log) %{_sbindir}/dloginit
%attr(-,log,log) %{_unitdir}/dloginit.service
%{_unitdir}/sysinit.target.wants/dloginit.service
%endif

%files -n libdlog-devel
%{_includedir}/dlog/dlog.h
%{_includedir}/dlog/dlog-internal.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so
