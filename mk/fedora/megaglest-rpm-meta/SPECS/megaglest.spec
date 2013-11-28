#
# spec file for package megaglest ( Version 3.5.2 )
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

Name:           megaglest
Version:        3.6.0
Release:        0.1
License:        GPLv2+
Summary:        Free 3D Real-Time Customizable Strategy Game
Url:            http://www.glest.org/
Group:          Amusements/Games/Strategy/Real Time
Source:         %{name}-source-%{version}.tar.bz2
Source1:        %{name}.desktop
Source2:        %{name}.png
Source3:        icons.tar.bz2
Source4:        README.SUSE
BuildRequires:  cmake
BuildRequires:  jam, libX11-devel, SDL-devel, openal-soft-devel, xerces-c-devel,freeglut-devel
BuildRequires:  krb5-devel, libdrm-devel, libidn-devel, libjpeg-devel, libpng-devel, libssh2-devel
BuildRequires:  openldap-devel,libxml2-devel, subversion, mesa-libGL-devel, mesa-libGLU-devel
BuildRequires:  openal-soft-devel, SDL-devel, libcurl-devel, c-ares-devel, wxGTK-devel,glew-devel
BuildRequires:  libogg-devel, libvorbis-devel, lua-devel, wxGTK-devel, openssl-devel, wxBase
BuildRequires:  desktop-file-utils, recode, gcc, gcc-c++,ftgl-devel,ftgl,autogen,autogen-libopts
Requires:       megaglest-data >= %{version}
Requires:       glx-utils
Requires:       opengl-games-utils
Requires:       megaglest-data >= %{version}
Obsoletes:      glest
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
MegaGlest takes place in a context that could be compared to that of
pre-Renaissance Europe with the license that magic forces exist in the
environment and can be controlled.

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
%setup -q -n %{name}-%{version}
tar xjf $RPM_SOURCE_DIR/icons.tar.bz2

%build
mkdir build
cd build
cmake -DCFLAGS=%{optflags} -DCXXFLAGS=$CFLAGS -DWANT_SVN_STAMP=OFF -DCMAKE_INSTALL_PREFIX=/usr ..
# unforce link against libcurl.a
find . -name link.txt -exec sed -ie 's!/usr/lib/libcurl.a!-lcurl!g' {} \;
find . -name link.txt -exec sed -ie 's!/usr/lib64/libcurl.a!-lcurl!g' {} \;
make

%install
cd mk/linux/
mkdir -p %{buildroot}%{_bindir}/
mkdir -p %{buildroot}%{_datadir}/%{name}/data/
install -m 755 megaglest %{buildroot}%{_bindir}/%{name}
install -m 755 megaglest_configurator %{buildroot}%{_bindir}/megaglest_configurator
install -m 755 megaglest_editor %{buildroot}%{_bindir}/megaglest_editor
install -m 755 megaglest_g3dviewer %{buildroot}%{_bindir}/megaglest_g3dviewer
install -m 644 $RPM_SOURCE_DIR/README.SUSE %{buildroot}%{_datadir}/%{name}/README.SUSE
cd ../../
install -m 644 glest.ini %{buildroot}%{_datadir}/%{name}/glest.ini
install -m 644 glestkeys.ini %{buildroot}%{_datadir}/%{name}/glestkeys.ini
#install -m 644 servers.ini %{buildroot}%{_datadir}/%{name}/servers.ini
mkdir -p %{buildroot}%{_datadir}/applications
install -m 644 $RPM_SOURCE_DIR/%{name}.desktop %{buildroot}%{_datadir}/applications/
mkdir -p %{buildroot}%{_datadir}/pixmaps
install -m 644 $RPM_SOURCE_DIR/%{name}.png %{buildroot}%{_datadir}/pixmaps

%clean
rm -rf %{buildroot}

%files
%defattr(-, root, root)
%{_bindir}/%{name}
%{_bindir}/megaglest_configurator
%{_bindir}/megaglest_editor
%{_bindir}/megaglest_g3dviewer
%dir %{_datadir}/%{name}/
%{_datadir}/%{name}/*
%{_datadir}/pixmaps/%{name}.png
%{_datadir}/applications/%{name}.desktop

%changelog
