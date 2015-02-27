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

%package -n dlogtests
Summary:    Runs test of dlog
Requires:   lib%{name} = %{version}-%{release}

%description -n dlogtests
Tests for dlog.

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

ln -s ./dlog@.service %{buildroot}%{_unitdir}/dlog@all.service
ln -s ./dlog@.service %{buildroot}%{_unitdir}/dlog@radio.service
ln -s ../dlog@.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog@all.service
ln -s ../dlog@.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog@radio.service


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
%attr(644,root,root) %manifest %{name}.manifest
%attr(644,root,root) %license LICENSE.APLv2
%attr(700,root,root) %{TZ_SYS_ETC}/dump.d/default.d/dlog_dump.sh
%attr(700,root,root) %{_bindir}/dlogutil
%attr(755,root,root) %{_sbindir}/dlogctrl
%attr(644,root,root) %{_unitdir}/dlog@.service
%{_unitdir}/dlog@all.service
%{_unitdir}/dlog@radio.service
%{_unitdir}/multi-user.target.wants/dlog@all.service
%{_unitdir}/multi-user.target.wants/dlog@radio.service
%attr(644,root,root) %{_libdir}/udev/rules.d/01-dlog.rules
%attr(644,root,root) %config(noreplace) %{_sysconfdir}/dlog/platformlog.conf
%attr(644,root,root) %config(noreplace) %{TZ_SYS_ETC}/dlog/dlog.all.conf
%attr(644,root,root) %config(noreplace) %{TZ_SYS_ETC}/dlog/dlog.radio.conf

%files  -n libdlog
%manifest %{name}.manifest
%{_libdir}/libdlog.so.*

%files -n libdlog-devel
%manifest %{name}.manifest
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

%files  -n dlogtests
%license LICENSE.APLv2
%attr(755,root,root) %{_bindir}/dlogtests

