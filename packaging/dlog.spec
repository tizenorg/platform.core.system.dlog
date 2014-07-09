Name:       dlog
Summary:    Logging service
Version:    0.4.1
Release:    0
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    dlog.manifest

BuildRequires: pkgconfig(libsystemd-journal)
BuildRequires: pkgconfig(libtzplatform-config)

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
Summary:    Manages dlog configuration
Requires:   lib%{name} = %{version}-%{release}

%description -n dlogutil
Utilities for managing dlog configuration

%prep
%setup -q
cp %{SOURCE1} .

%build
%reconfigure --disable-static
%__make %{?jobs:-j%jobs}

%install
%make_install
mkdir -p %{buildroot}%{TZ_SYS_ETC}/dlog
cp platformlog.conf %{buildroot}%{TZ_SYS_ETC}/dlog/platformlog.conf
cp dlogctrl %{buildroot}%{TZ_SYS_BIN}/dlogctrl

%post -n libdlog -p /sbin/ldconfig

%postun -n libdlog -p /sbin/ldconfig

%files  -n libdlog
%manifest %{name}.manifest
%config %{TZ_SYS_ETC}/dlog/platformlog.conf
%{_libdir}/libdlog.so.*

%files -n libdlog-devel
%manifest %{name}.manifest
%{_includedir}/dlog/dlog.h
%{_libdir}/pkgconfig/dlog.pc
%{_libdir}/libdlog.so

%files -n dlogutil
%manifest %{name}.manifest
%license LICENSE.APLv2
%attr(750,system,app_logging) %{_bindir}/dlogctrl
