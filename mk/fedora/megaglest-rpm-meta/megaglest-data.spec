#
# spec file for package megaglest-data (Version 3.5.2 )
#
# Copyright 2008 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative
#  
# Please submit bugfixes or comments via http://bugs.opensuse.org  
#

# norootforbuild

Name:           megaglest-data
Version:        3.6.0
Release:        0.1
License:        Any permissive
Summary:        Data files for MegaGlest
Url:            http://www.megaglest.org/
Group:          Amusements/Games/Strategy/Real Time
Source:         %{name}-%{version}.tar.bz2
Requires:       megaglest >= %{version}
%if 0%{?suse_version}
BuildRequires:  fdupes
%endif
Obsoletes:      glest-data
Obsoletes:      glest-megapack
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildArch:      noarch

%description
Data files required for playing MegaGlest.

Authors:
--------
    MARTINO FIGUEROA <game@glest.org>
    JOSE GONZALEZ <game@glest.org>
    TUCHO FERNANDEZ <game@glest.org>
    JOSE ZANNI <game@glest.org>
    MARCOS CARUNCHO <game@glest.org>
    Matthias Braun <matze@braunis.de>
    Karl Robillard <krobbillard@san.rr.com>
    Titus Tscharntke <info@titusgames.de>
    Mark Vejvoda <mark_vejvoda@hotmail.com
 
%prep
%setup -n . -D -T
 
%build
exit 0
 
%install
mkdir -p %{buildroot}%{_datadir}/megaglest
pushd %{buildroot}%{_datadir}/megaglest
  tar xjf %{SOURCE0}
# get rid of unwanted files
  find . -name "*~" -exec rm {} \;
  find . -name "*\.bak" -exec rm {} \;
  for i in $(find . -name "*.G3D"); do
    rename G3D g3d $i
  done
popd
 
iconv -f iso-8859-15 -t utf-8 %{buildroot}%{_datadir}/megaglest/docs/README.data-license.txt > README
%if 0%{?suse_version}
%fdupes -s $RPM_BUILD_ROOT
%endif
 
%clean
rm -rf %{buildroot}
 
%files
%defattr(-, root, root)
%doc README
%{_datadir}/megaglest/
 
%changelog
