%define name cmph
%define version 0.2
%define release 1

Name: %{name}
Version: %{version}
Release: %{release}
Summary: C Minimal perfect hash library
Source: %{name}-%{version}.tar.gz
License: Proprietary
URL: http://www.akwan.com.br
BuildArch: i386
Group: Sitesearch
BuildRoot: %{_tmppath}/%{name}-root

%description
C Minimal perfect hash library

%prep
rm -Rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_ROOT
%setup
mkdir $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT/usr
CXXFLAGS="-O2" ./configure --prefix=/usr/

%build
make

%install
DESTDIR=$RPM_BUILD_ROOT make install

%files
%defattr(755,root,root)
/

%changelog
* Tue Jun 1 2004 Davi de Castro Reis <davi@akwan.com.br>
+ Initial build
