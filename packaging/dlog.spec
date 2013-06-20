Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    15
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  dlog-main.service
Source102:  dlog-radio.service
Source103:  dlog.manifest
Source104:  tizen-debug-level.service

BuildRequires: pkgconfig(libsystemd-journal)
Requires(post): coreutils

%description
dlog API library

%package -n libdlog
Summary:    Logging service dlog API
Group:      Development/Libraries
Requires(post): smack-utils

%description -n libdlog
dlog API library

%package -n libdlog-devel
Summary:    Logging service dlog API
Group:      Development/Libraries
Requires:   lib%{name} = %{version}-%{release}

%description -n libdlog-devel
dlog API library


%package -n dlogutil
Summary:    Print log data to the screen
Group:      Development/Libraries
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
%autogen --disable-static
%configure --disable-static --without-systemd-journal
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/opt/etc/dump.d/default.d
cp %{_builddir}/%{name}-%{version}/dlog_dump.sh %{buildroot}/opt/etc/dump.d/default.d/dlog_dump.sh
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc3.d
rm -f %{buildroot}/%{_sysconfdir}/etc/rc.d/rc3.d/S01dlog
ln -s ../init.d/dlog.sh %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S01dlog

mkdir -p %{buildroot}%{_unitdir}/basic.target.wants
mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants

install -m 0644 %SOURCE101 %{buildroot}%{_unitdir}
install -m 0644 %SOURCE102 %{buildroot}%{_unitdir}
install -m 0644 %SOURCE104 %{buildroot}%{_unitdir}

ln -s ../dlog-main.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog-main.service
ln -s ../dlog-radio.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog-radio.service
ln -s ../tizen-debug-level.service %{buildroot}%{_unitdir}/basic.target.wants/tizen-debug-level.service

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}

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

%post -n libdlog
/sbin/ldconfig
echo 0 > /opt/etc/platformlog.conf
chmod 660 /opt/etc/platformlog.conf
chown root:app_logging /opt/etc/platformlog.conf
if [ -f %{_libdir}/rpm-plugins/msm.so ]; then
	chsmack -a '*' -e 'none' /opt/etc/platformlog.conf
fi
%postun -n libdlog
/sbin/ldconfig
rm /opt/etc/platformlog.conf

%files  -n dlogutil
%manifest %{name}.manifest
/usr/share/license/%{name}
%doc LICENSE.APLv2
%attr(755,root,root) /opt/etc/dump.d/default.d/dlog_dump.sh
%attr(755,root,app_logging) %{_bindir}/dlogutil
%attr(755,root,app_logging) %{_bindir}/dlogctrl
%{_sysconfdir}/rc.d/init.d/dlog.sh
%{_sysconfdir}/rc.d/rc3.d/S05dlog
%{_unitdir}/tizen-debug-level.service
%{_unitdir}/dlog-main.service
%{_unitdir}/dlog-radio.service
%{_unitdir}/basic.target.wants/tizen-debug-level.service
%{_unitdir}/multi-user.target.wants/dlog-main.service
%{_unitdir}/multi-user.target.wants/dlog-radio.service
%attr(775,root,app_logging) %dir /opt/etc/dlog

%files  -n libdlog
%manifest %{name}.manifest
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0

%files -n libdlog-devel
%manifest %{name}.manifest
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

