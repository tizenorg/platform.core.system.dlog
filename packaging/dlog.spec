Name:       dlog
Summary:    Logging service
Version:    0.4.0
Release:    5.1
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
Source0:    dlog-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig


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

%post -n dlogutil
#Add boot sequence script
rm -f /etc/rc.d/rc3.d/S05dlog /etc/rc.d/rc5.d/S05dlog
ln -s /etc/rc.d/init.d/dlog.sh /etc/rc.d/rc3.d/S05dlog
ln -s /etc/rc.d/init.d/dlog.sh /etc/rc.d/rc5.d/S05dlog


%post -n libdlog -p /sbin/ldconfig

%postun -n libdlog -p /sbin/ldconfig


%files  -n dlogutil
%{_bindir}/dlogutil
%{_sysconfdir}/rc.d/init.d/dlog.sh

%files  -n libdlog
%doc LICENSE
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0

%files -n libdlog-devel
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

