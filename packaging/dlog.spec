Name:       dlog
Summary:    Logging service
Version:    0.4.0
Release:    5.1
Group:      TO_BE/FILLED_IN
License:    Apache License
Source0:    %{name}-%{version}.tar.gz
Source101:  packaging/dlog-main.service
Source102:  packaging/dlog-radio.service
BuildRequires: pkgconfig(systemd)
Requires(post): /sbin/ldconfig
Requires(post): /usr/bin/systemctl
Requires(post): /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
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
Requires:   lib%{name} = %{version}-%{release}

%description -n libdlog-devel
dlog API library


%package -n dlogutil
Summary:    print log data to the screen
Group:      Development/Libraries
Requires:   lib%{name} = %{version}-%{release}
Requires(post): /bin/rm, /bin/ln

%description -n dlogutil
utilities for print log data



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
mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc5.d
rm -f %{buildroot}/%{_sysconfdir}/etc/rc.d/rc3.d/S05dlog
rm -f %{buildroot}/%{_sysconfdir}/etc/rc.d/rc5.d/S05dlog
ln -s ../init.d/dlog.sh %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S05dlog
ln -s ../init.d/dlog.sh %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/S05dlog

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants

install -m 0644 %SOURCE101 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE102 %{buildroot}%{_libdir}/systemd/system/

ln -s ../dlog-main.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/dlog-main.service
ln -s ../dlog-radio.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/dlog-radio.service

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}


%preun -n dlogutil
if [ $1 == 0 ]; then
    systemctl stop dlog-main.service
    systemctl stop dlog-radio.service
fi

%post -n dlogutil
mkdir -p /opt/etc/dlog
chown 0:5000 /opt/etc/dlog
chmod 775 /opt/etc/dlog
chmod 755 /usr/bin/dlogctrl
systemctl daemon-reload
if [ $1 == 1 ]; then
    systemctl restart dlog-main.service
    systemctl restart dlog-radio.service
fi

%postun -n dlogutil
systemctl daemon-reload

%post -n libdlog
/sbin/ldconfig
%postun -n libdlog
/sbin/ldconfig

%files  -n dlogutil
%manifest dlogutil.manifest
%{_bindir}/dlogutil
%{_bindir}/dlogctrl
%{_sysconfdir}/rc.d/init.d/dlog.sh
%{_sysconfdir}/rc.d/rc3.d/S05dlog
%{_sysconfdir}/rc.d/rc5.d/S05dlog
%{_libdir}/systemd/system/dlog-main.service
%{_libdir}/systemd/system/dlog-radio.service
%{_libdir}/systemd/system/multi-user.target.wants/dlog-main.service
%{_libdir}/systemd/system/multi-user.target.wants/dlog-radio.service

%files  -n libdlog
/usr/share/license/%{name}
%doc LICENSE
/opt/etc/.dloglevel
/etc/profile.d/tizen_platform_env.sh
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0

%files -n libdlog-devel
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

