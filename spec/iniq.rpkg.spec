Name: {{{ git_cwd_name name="iniq" }}}
Version: {{{ git_cwd_version lead="$(git tag | sed -n 's/^v//p' | sort --version-sort -r | head -n1)" }}}
Release: 1%{?dist}
Summary: A simple INI file reader for the command line

License: BSD-3-Clause
URL: https://github.com/jcrd/iniq
VCS: {{{ git_cwd_vcs }}}
Source0: {{{ git_cwd_pack }}}

BuildRequires: gcc
BuildRequires: perl

%global debug_package %{nil}

%description
iniq is a simple INI file reader for the command line. It queries an INI file based on the path <section><separator><key> and allows use of custom separators in the file and formatting of the output. Sections inherit keys from a special DEFAULT section unless the -D flag is used.

%prep
{{{ git_cwd_setup_macro }}}

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
{{{ git_cwd_changelog }}}
