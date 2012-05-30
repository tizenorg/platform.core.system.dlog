Name:       dlog
Summary:    Logging service
Version:	0.4.1
Release:    5.1
Group:      TO_BE/FILLED_IN
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/dlog.manifest 
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
Requires(post): /bin/rm, /bin/ln

%description -n dlogutil
utilities for print log data



%prep
%setup -q 


%build
cp %{SOURCE1001} .
%autogen
%configure 
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post -n dlogutil
#Add boot sequence script
mkdir -p /etc/rc.d/rc5.d
rm -f /etc/rc.d/rc3.d/S05dlog /etc/rc.d/rc5.d/S05dlog
ln -s /etc/rc.d/init.d/dlog.sh /etc/rc.d/rc3.d/S05dlog
ln -s /etc/rc.d/init.d/dlog.sh /etc/rc.d/rc5.d/S05dlog


%post -n libdlog -p /sbin/ldconfig

%postun -n libdlog -p /sbin/ldconfig


%files  -n dlogutil
%manifest dlog.manifest
%{_bindir}/dlogutil
%{_sysconfdir}/rc.d/init.d/dlog.sh

%files  -n libdlog
%manifest dlog.manifest
%doc LICENSE
%{_libdir}/libdlog.so.0
%{_libdir}/libdlog.so.0.0.0

%files -n libdlog-devel
%manifest dlog.manifest
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so
