Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    5
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  dlog-main.service
Source102:  dlog-radio.service
Source103:  dlogutil.manifest
Source104:  dlog.manifest
Source105:  packaging/libdlog.manifest
Source106:  tizen-debug-level.service

BuildRequires: pkgconfig(libsystemd-journal)
Requires(post): /usr/bin/vconftool
Requires(post): coreutils

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


%build
%autogen --disable-static
%configure --disable-static
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
cp %{SOURCE103} .
cp %{SOURCE104} .
cp %{SOURCE105} .
%make_install
mkdir -p %{buildroot}/opt/etc/dlog
cp %{_builddir}/%{name}-%{version}/.dloglevel %{buildroot}/opt/etc/dlog/.dloglevel
mkdir -p %{buildroot}/etc/profile.d/
cp %{_builddir}/%{name}-%{version}/tizen_platform_env.sh %{buildroot}/etc/profile.d/tizen_platform_env.sh
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc3.d
rm -f %{buildroot}/%{_sysconfdir}/etc/rc.d/rc3.d/S05dlog
ln -s ../init.d/dlog.sh %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S05dlog

mkdir -p %{buildroot}%{_unitdir}/basic.target.wants
mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants

install -m 0644 %SOURCE101 %{buildroot}%{_unitdir}
install -m 0644 %SOURCE102 %{buildroot}%{_unitdir}
install -m 0644 %SOURCE106 %{buildroot}%{_unitdir}

ln -s ../dlog-main.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog-main.service
ln -s ../dlog-radio.service %{buildroot}%{_unitdir}/multi-user.target.wants/dlog-radio.service
ln -s ../tizen-debug-level.service %{buildroot}%{_unitdir}/basic.target.wants/tizen-debug-level.service

mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}/opt/etc/dlog

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
/usr/share/license/%{name}
%doc LICENSE.APLv2
%attr(755,root,root) /opt/etc/dlog/.dloglevel
%attr(755,root,root) /etc/profile.d/tizen_platform_env.sh
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

