Name: magnum-extras
Version: 2020.06.496.gff88fb6
Release: 1
Summary: Extras for the Magnum C++11 graphics engine
License: MIT
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires: magnum, magnum-plugins
BuildRequires: cmake, git, gcc-c++

%description
Here you find extra functionality for the Magnum C++11 graphics engine -
playground for testing new APIs, specialized stuff that doesn't necessarily
need to be a part of main Magnum repository or mutually exclusive functionality.

%package devel
Summary: Magnum Extras development files
Requires: %{name} = %{version}, magnum-devel

%description devel
Headers and tools needed for extra functionality for the Magnum C++11 graphics engine.

%prep
%setup -c -n %{name}-%{version}

%build
mkdir build && cd build
# Configure CMake
cmake ../%{name}-%{version} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DMAGNUM_WITH_PLAYER=ON \
  -DMAGNUM_WITH_UI=ON \
  -DMAGNUM_WITH_UI_GALLERY=ON

make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
cd build
make DESTDIR=$RPM_BUILD_ROOT install
strip $RPM_BUILD_ROOT/%{_libdir}/*.so*
strip $RPM_BUILD_ROOT/%{_bindir}/*

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_libdir}/*.so*

%doc %{name}-%{version}/COPYING

%files devel
%defattr(-,root,root,-)
%{_bindir}/*
%{_includedir}/Magnum
%{_datadir}/cmake/MagnumExtras
%{_datadir}/applications/magnum-player.desktop

%changelog
# TODO: changelog
