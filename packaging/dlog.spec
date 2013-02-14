Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    4
Group:      System/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source101:  dlog-main.service
Source102:  dlog-radio.service
Source103:  tizen-debug-level.service
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
Summary:    print log data to the screen
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
%make_install
mkdir -p %{buildroot}/opt/etc/
cp %{_builddir}/%{name}-%{version}/.dloglevel %{buildroot}/opt/etc/.dloglevel
mkdir -p %{buildroot}/etc/profile.d/
cp %{_builddir}/%{name}-%{version}/tizen_platform_env.sh %{buildroot}/etc/profile.d/tizen_platform_env.sh
mkdir -p %{buildroot}/usr/bin/
cp %{_builddir}/%{name}-%{version}/dlogctrl %{buildroot}/usr/bin/dlogctrl

mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc3.d
rm -f %{buildroot}/%{_sysconfdir}/etc/rc.d/rc3.d/S05dlog
ln -s ../init.d/dlog.sh %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S05dlog

mkdir -p %{buildroot}%{_libdir}/systemd/system/basic.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants

install -m 0644 %SOURCE101 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE102 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE103 %{buildroot}%{_libdir}/systemd/system/

ln -s ../dlog-main.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/dlog-main.service
ln -s ../dlog-radio.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/dlog-radio.service
ln -s ../tizen-debug-level.service %{buildroot}%{_libdir}/systemd/system/basic.target.wants/tizen-debug-level.service

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
%manifest dlogutil.manifest
%{_bindir}/dlogutil
%attr(775,root,root) %{_bindir}/dlogctrl
%{_sysconfdir}/rc.d/init.d/dlog.sh
%{_sysconfdir}/rc.d/rc3.d/S05dlog
%{_libdir}/systemd/system/tizen-debug-level.service
%{_libdir}/systemd/system/dlog-main.service
%{_libdir}/systemd/system/dlog-radio.service
%{_libdir}/systemd/system/basic.target.wants/tizen-debug-level.service
%{_libdir}/systemd/system/multi-user.target.wants/dlog-main.service
%{_libdir}/systemd/system/multi-user.target.wants/dlog-radio.service
%attr(775,root,app) %dir /opt/etc/dlog

%files  -n libdlog
/usr/share/license/%{name}
%doc LICENSE.APLv2
/opt/etc/.dloglevel
/etc/profile.d/tizen_platform_env.sh
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0

%files -n libdlog-devel
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

