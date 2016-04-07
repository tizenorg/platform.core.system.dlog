Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    17
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  packaging/dlogutil.manifest
Source102:  packaging/libdlog.manifest
Source201:  packaging/dlog.conf.in
Source202:  packaging/dlog_logger.conf.in
Source203:  packaging/dlog_logger.conf-micro.in
Source204:  packaging/dlog_logger.conf-debug.in
Source301:  packaging/dlog_logger.service
Source302:  packaging/dlog_logger.path.kmsg
Source303:	packaging/dlog_logger.path.logger
Source401:  packaging/dloginit.service
Source501:  packaging/01-dlog.rules.kmsg
Source502:	packaging/01-dlog.rules.logger

%if 0%{!?backend_kmsg:1}
%define backend_kmsg 0
%endif

%if 0%{!?backend_journal:1}
%define backend_journal 0
%endif

%if 0%{!?backend_logger:1}
%define backend_logger 0
%endif

%if 0%{!?backend_pipe:1}
%define backend_pipe 0
%endif

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

%prep
%setup -q

%build
cp %{SOURCE101} .
cp %{SOURCE102} .
%autogen --disable-static
%configure --disable-static \
			--enable-fatal_on \
		%if %{?backend_journal} 
			--enable-journal \
		%endif
		%if %{?backend_pipe}
			--enable-pipe \
		%endif
                %if %{?backend_kmsg}
                        --enable-kmsg \
                %endif
                %if %{?backend_logger}
                        --enable-android-logger \
                %endif
			--enable-engineer_mode
make %{?jobs:-j%jobs} \
	CFLAGS+=-DKMSG_DEV_CONFIG_FILE=\\\"/run/dloginit.conf\\\" \
%if %{backend_journal}
	CFLAGS+=-DDLOG_BACKEND_JOURNAL
%endif
%if %{backend_kmsg}
	CFLAGS+=-DDLOG_BACKEND_KMSG
%endif
%if %{backend_logger}
	CFLAGS+=-DDLOG_BACKEND_LOGGER
%endif
%if %{backend_pipe}
	CFLAGS+=-DDLOG_BACKEND_PIPE
%endif


%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/scripts/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}/opt/etc
cp %SOURCE201 %{buildroot}/opt/etc/dlog.conf

mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants/

%if %{backend_kmsg} || %{backend_logger}

install -m 0644 %SOURCE301 %{buildroot}%{_unitdir}

%if %{backend_kmsg}
install -m 0644 %SOURCE302 %{buildroot}%{_unitdir}/dlog_logger.path
%else
install -m 0644 %SOURCE303 %{buildroot}%{_unitdir}/dlog_logger.path
%endif

ln -s ../dlog_logger.path %{buildroot}%{_unitdir}/multi-user.target.wants/dlog_logger.path

# default set log output to external files
cp %SOURCE202 %{buildroot}/opt/etc/dlog_logger.conf

%endif

mkdir -p %{buildroot}%{_unitdir}/sysinit.target.wants/
install -m 0644 %SOURCE401 %{buildroot}%{_unitdir}
ln -s ../dloginit.service %{buildroot}%{_unitdir}/sysinit.target.wants/dloginit.service

mkdir -p %{buildroot}%{_udevrulesdir}

%if %{backend_kmsg}
install -m 0644 %SOURCE501 %{buildroot}%{_udevrulesdir}/01-dlog.rules
%else
install -m 0644 %SOURCE502 %{buildroot}%{_udevrulesdir}/01-dlog.rules
%endif

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/%{name}
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/libdlog
cp LICENSE.Apache-2.0 %{buildroot}/usr/share/license/dlogutil

mkdir -p %{buildroot}/var/log/dlog

# Workaround: replace with dlogutil script due to scheduling issue
#cp %{_builddir}/%{name}-%{version}/scripts/dlogutil.sh %{buildroot}/usr/bin/dlogutil

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
%{_udevrulesdir}/01-dlog.rules
%if %{backend_pipe} || %{backend_logger} || %{backend_kmsg}
%attr(750,log,log) %{_bindir}/dlog_logger
%if %{backend_logger} || %{backend_kmsg}
%attr(664,log,log) /opt/etc/dlog_logger.conf
%{_unitdir}/dlog_logger.service
%{_unitdir}/dlog_logger.path
%{_unitdir}/multi-user.target.wants/dlog_logger.path
%endif
%endif

%files  -n libdlog
%manifest libdlog.manifest
/usr/share/license/libdlog
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0
%attr(664,log,log) /opt/etc/dlog.conf
/usr/share/license/%{name}
%attr(700,log,log) %{_sbindir}/dloginit
%attr(-,log,log) %{_unitdir}/dloginit.service
%{_unitdir}/sysinit.target.wants/dloginit.service

%files -n libdlog-devel
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so
