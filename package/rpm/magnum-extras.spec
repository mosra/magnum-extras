Name:           magnum-extras
Version:        v2020.06
Release:        69ecb84%{?dist}
Summary:        Extras for the Magnum C++11/C++14 graphics engine

License:        MIT
URL:            https://magnum.graphics/
Source0:        %{name}-%{version}-%{release}.tar.gz
Requires:       magnum

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.4.0
BuildRequires:  magnum-devel

%description
Extras for the Magnum C++11/C++14 graphics engine

%package devel
Summary:        Extras for the Magnum C++11/C++14 graphics engine
Requires:       magnum-devel
Requires:       %{name} = %{version}

%description devel
Extras for the Magnum C++11/C++14 graphics engine

%global debug_package %{nil}

%prep
%autosetup


%build
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_PLAYER=ON \
    -DWITH_UI=ON \
    -DWITH_UI_GALLERY=ON
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
cd build
make DESTDIR=$RPM_BUILD_ROOT install
strip $RPM_BUILD_ROOT/%{_prefix}/lib*/* $RPM_BUILD_ROOT/%{_prefix}/bin/*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%{_prefix}/bin/*
%{_prefix}/lib*/lib*.so.*
%{_prefix}/share/applications/magnum-player.desktop
%doc COPYING

%files devel
%{_prefix}/include/Magnum/*
%{_prefix}/lib*/lib*.so
%{_prefix}/share/cmake*
%doc COPYING


%changelog
* Sun Oct 17 2021 1b00 <1b00@pm.me> - v2020.06-69ecb84
- Initial release.
