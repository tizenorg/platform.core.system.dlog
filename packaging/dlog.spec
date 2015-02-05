%bcond_with dlog_to_systemd_journal

Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  dlog-main.service
Source102:  dlog-radio.service
Source103:  dlog.manifest

%if %{with dlog_to_systemd_journal}
BuildRequires: pkgconfig(libsystemd-journal)
%endif
BuildRequires:	pkgconfig(libtzplatform-config)
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
Logging service dlog API library


%package -n dlogutil
Summary:    Print log data to the screen
Requires:   lib%{name} = %{version}-%{release}
Requires(post): /usr/bin/systemctl
Requires(postun): /usr/bin/systemctl
Requires(preun): /usr/bin/systemctl

%description -n dlogutil
Utilities for print log data

%prep
%setup -q
cp %{SOURCE103} .

%build
%reconfigure --disable-static \
%if %{with dlog_to_systemd_journal}
--with-systemd-journal
%else
--without-systemd-journal
%endif

make %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}%{TZ_SYS_ETC}/dump.d/default.d
cp %{_builddir}/%{name}-%{version}/dlog_dump.sh %{buildroot}%{TZ_SYS_ETC}/dump.d/default.d/dlog_dump.sh
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}%{_unitdir}/basic.target.wants
mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants

install -m 0644 %SOURCE101 %{buildroot}%{_unitdir}
install -m 0644 %SOURCE102 %{buildroot}%{_unitdir}

ln -s ../dlog-main.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog-main.service
ln -s ../dlog-radio.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog-radio.service


%preun -n dlogutil
if [ $1 == 0 ]; then
    systemctl stop dlog-main.service
    systemctl stop dlog-radio.service
fi

%post -n dlogutil
systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart dlog-main.service
    systemctl restart dlog-radio.service
fi

%postun -n dlogutil
systemctl daemon-reload

%post -n libdlog -p /sbin/ldconfig

%postun -n libdlog -p /sbin/ldconfig

%files  -n dlogutil
%manifest %{name}.manifest
%license LICENSE.APLv2
%attr(755,root,root) %{TZ_SYS_ETC}/dump.d/default.d/dlog_dump.sh
%attr(755,root,app_logging) %{_bindir}/dlogutil
%attr(755,root,app_logging) %{_bindir}/dlogctrl
%{_unitdir}/dlog-main.service
%{_unitdir}/dlog-radio.service
%{_unitdir}/multi-user.target.wants/dlog-main.service
%{_unitdir}/multi-user.target.wants/dlog-radio.service

%files  -n libdlog
%manifest %{name}.manifest
%{_libdir}/libdlog.so.*

%files -n libdlog-devel
%manifest %{name}.manifest
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

