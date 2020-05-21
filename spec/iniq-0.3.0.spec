Name: iniq
Version: 0.3.0
Release: 1%{?dist}
Summary: A simple INI file reader for the command line

License: BSD-3-Clause
URL: https://github.com/jcrd/iniq
Source0: https://github.com/jcrd/iniq/archive/v0.3.0.tar.gz

BuildRequires: gcc
BuildRequires: perl

%global debug_package %{nil}

%description
iniq is a simple INI file reader for the command line. It queries an INI file based on the path <section><separator><key> and allows use of custom separators in the file and formatting of the output. Sections inherit keys from a special DEFAULT section unless the -D flag is used.

%prep
%setup

%build
%make_build PREFIX=/usr

%install
%make_install PREFIX=/usr

%files
%license LICENSE
%doc README.md
/usr/bin/%{name}
/usr/share/man/man1/%{name}.1.gz

%changelog
* Mon May 11 2020 James Reed <jcrd@tuta.io> - 0.3.0
- Initial package
