# -----------------------------------------------------
# Define static values
# ----------------------------------------------------- 

# -----------------------------------------------------
# Define requirements for configure, BuildRequire,
# Require and so on (depending on macros)
# -----------------------------------------------------

# -----------------------------------------------------
# RPM-Information
# -----------------------------------------------------
Name:			jhead
Summary:		Tool for handling EXIF metadata in JPEG image files

Version:		2.2
Release:		0

Group:			Libraries
License:		Public Domain
URL:			http://www.sentex.net/~mwandel/jhead/

Source:			http://www.sentex.net/~mwandel/jhead/%{name}-%{version}.tar.gz

BuildRoot:		%{_tmppath}/%{name}-%{version}-root
BuildRequires:		glibc-devel

# -----------------------------------------------------
# Package-Information (for main package)
# -----------------------------------------------------
%description
This package provides a tool for displaying and manipulating non-image
portions of EXIF format JPEG image files, as produced by most digital cameras.

# -----------------------------------------------------
# Prepare-Section
# -----------------------------------------------------
%prep
%setup -q 

# -----------------------------------------------------
# Build-Section
# -----------------------------------------------------
%build
make %{?_smp_mflags}

# -----------------------------------------------------
# Install-Section
# -----------------------------------------------------
%install
mkdir -p %{buildroot}%{_bindir}
cp jhead %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_mandir}/man1/
cp jhead.1.gz %{buildroot}%{_mandir}/man1/

# -----------------------------------------------------
# Clean-Section
# -----------------------------------------------------
%clean
rm -rf %{buildroot}

# -----------------------------------------------------
# Files-Section (for main package)
# -----------------------------------------------------
%files
%defattr(0644,root,root,0755)
%attr(0755,root,root) %{_bindir}/*
%doc readme.txt usage.html
%{_mandir}/man1/jhead.1.gz

# -----------------------------------------------------
# Changelog-Section
# -----------------------------------------------------
%changelog
* Tue Jan 08 2004 Matthias Wandel <mwandel[at]sentex.net> - 2.0-0
- Bumped version number to 2.1 for new jhead release.

* Tue Jun 03 2003 Oliver Pitzeier <oliver@linux-kernel.at> - 2.0-3
- Specfile cleanup/beautifying
- Use _smp_mflags within make
- Add versions to the changelog entries

* Mon Apr 14 2003 Matthias Wandel <mwandel[at]sentex.net> - 2.0-2
- First jhead 2.0 RPM built by me.
- Finally wrote a nice man page for jhead
- Using jhead 1.9 RPM from connectiva linux as starting point (left in the portugese tags)
  
* Jan 2004
- Added -cl feature
- added -noroot feature

* Jun 2004
- Various bug and spelling fixes.
- added ability to do sequential renaming
