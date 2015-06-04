%bcond_with dlog_to_systemd_journal

Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  dlog@.service
Source102:  dlog.manifest

%if %{with dlog_to_systemd_journal}
BuildRequires: pkgconfig(libsystemd-journal)
%endif
BuildRequires: pkgconfig(libtzplatform-config)
Requires: libtzplatform-config

%description
Logging service dlog API library

%package -n libdlog
Summary:    Logging service dlog API

%description -n libdlog
Logging service dlog API library

%package -n libdlog-devel
Summary:    Logging service dlog API
Requires:   lib%{name} = %{version}-%{release}

%description -n libdlog-devel
Logging service dlog API library - files for development

%package -n dlogutil
Summary:    Print log data to the screen
Requires:   lib%{name} = %{version}-%{release}
Requires(post): /usr/bin/systemctl
Requires(postun): /usr/bin/systemctl
Requires(preun): /usr/bin/systemctl

%description -n dlogutil
A tool for reading logs.


%prep
%setup -q
cp %{SOURCE102} .

%build
%reconfigure --disable-static \
%if %{with dlog_to_systemd_journal}
--with-systemd-journal
%else
--without-systemd-journal
%endif

%__make %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}%{TZ_SYS_ETC}/dump.d/default.d
cp dlog_dump.sh %{buildroot}%{TZ_SYS_ETC}/dump.d/default.d/dlog_dump.sh

mkdir -p %{buildroot}%{_unitdir}/basic.target.wants
mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants

install -m 0644 %SOURCE101 %{buildroot}%{_unitdir}



%preun -n dlogutil

%post -n dlogutil
systemctl daemon-reload

%postun -n dlogutil
systemctl daemon-reload

%post -n libdlog -p /sbin/ldconfig

%postun -n libdlog -p /sbin/ldconfig

%files  -n dlogutil
%attr(644,root,root) %manifest %{name}.manifest
%attr(644,root,root) %license LICENSE.APLv2
%attr(700,root,root) %{TZ_SYS_ETC}/dump.d/default.d/dlog_dump.sh
%attr(700,root,root) %{_bindir}/dlogutil
%attr(644,root,root) %{_unitdir}/dlog@.service

%files  -n libdlog
%manifest %{name}.manifest
%{_libdir}/libdlog.so.*

%files -n libdlog-devel
%manifest %{name}.manifest
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so


