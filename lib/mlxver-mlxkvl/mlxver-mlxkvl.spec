Name: mlxver-mlxkvl
Summary: Mellanox kernel space testing & VL library
Version: 1.0.0
Release: 1
Source: %{name}-%{version}.tar.gz
Vendor: Mellanox Technologies
License: GPL
ExclusiveOS: linux
Group: System Environment/Kernel
Provides: %{name}
URL: http://mellanox.com
BuildRoot: %{_tmppath}/%{name}-%{version}-root
# do not generate debugging packages by default - newer versions of rpmbuild
# may instead need:
#%define debug_package %{nil}
%debug_package %{nil}
# macros for finding system files to update at install time (pci.ids, pcitable)
%define find() %(for f in %*; do if [ -e $f ]; then echo $f; break; fi; done)
Requires: fileutils, findutils, gawk, bash

%description
This package contains the Linux  Mellanox testing KVL driver 

%prep
%setup

%build
cd src 
make clean
make 
cd -

%install
cd src
make INSTALL_MOD_PATH=%{buildroot} MANDIR=%{_mandir} install
cd -
%clean
rm -rf %{buildroot}

%files 
#-f file.list
%defattr(-,root,root)
/usr/src
/usr/include/mlxkvl/
/lib/modules
/usr/bin
%{_mandir}/man7/mlxkvl.7.gz
%doc COPYING
%doc README
#%doc file.list

%post

uname -r | grep BOOT || /sbin/depmod -a > /dev/null 2>&1 || true

%preun

%postun
uname -r | grep BOOT || /sbin/depmod -a > /dev/null 2>&1 || true

