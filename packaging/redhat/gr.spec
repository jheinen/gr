%define THIRDPARTY build/3rdparty
%define THIRDPARTY_SRC %{THIRDPARTY}/src
%define THIRDPARTY_INC %{THIRDPARTY}/include
%define THIRDPARTY_LIB %{THIRDPARTY}/lib

%if 0%{?__jcns} || 0%{?mlz}
%define fixedversion %{version}
%else
# use fixedversion for builds on build.opensuse.org - needed for deb builds.
%define fixedversion fixed
%define compression gz
%endif

%define qmake_qt4 qmake-qt4
%define grdir %{_prefix}/gr

Name:				gr
Summary:			GR, a universal framework for visualization applications
Version:			0.51.1
Release:			3%{?dist}
License:			MIT
Group:				Development/Libraries
Source:				gr-%{fixedversion}.tar%{?compression:.%{compression}}
Source1:			https://gr-framework.org/downloads/3rdparty/libogg-1.3.2.tar.gz
Source2:			https://gr-framework.org/downloads/3rdparty/libtheora-1.1.1.tar.gz
Source3:			https://gr-framework.org/downloads/3rdparty/libvpx-1.4.0.tar.bz2
Source4:			https://gr-framework.org/downloads/3rdparty/ffmpeg-4.2.1.tar.gz
Source5:			https://gr-framework.org/downloads/3rdparty/glfw-3.3.3.tar.gz
Source6:			https://gr-framework.org/downloads/3rdparty/zeromq-4.3.4.tar.gz
Source7:			https://gr-framework.org/downloads/3rdparty/cmake-3.6.3-Linux-x86_64.tar.gz
Source8:			https://gr-framework.org/downloads/3rdparty/cairo-1.14.6.tar.xz
Source9:			https://gr-framework.org/downloads/3rdparty/pixman-0.34.0.tar.gz
Source10:			https://gr-framework.org/downloads/3rdparty/tiff-4.0.10.tar.gz
Source11:			https://gr-framework.org/downloads/3rdparty/libopenh264-2.0.0.tar.gz
BuildRequires:		git
BuildRequires:		gcc-c++
BuildRequires:		libX11-devel
BuildRequires:		libXt-devel
BuildRequires:		libXft-devel
BuildRequires:		gtk2-devel
%if 0%{?__jcns}
%define debug_package %{nil}
%define qmake_qt4 %{_prefix}/qt4/bin/qmake
%define qmake_qt5 %{_prefix}/qt5/bin/qmake
BuildRequires: qt4-local
BuildRequires: qt5-local
BuildRequires: gcc-local
BuildRequires: cmake-local
%else
%if 0%{?centos_version} < 800
BuildRequires: qt-devel
%endif
%endif

%if 0%{?suse_version}
%define qmake_qt4 qmake
%define qmake_qt5 qmake-qt5
BuildRequires:		xorg-x11-devel
BuildRequires:		Mesa-libGL-devel
BuildRequires:		libqt4-devel
BuildRequires:		libqt5-qtbase-devel
%else
BuildRequires:		xorg-x11-server-devel
BuildRequires:		mesa-libGL-devel
%endif

%if 0%{?rhel_version}
BuildRequires:		ghostscript
%else
# RHEL: unresolvable package(s)
BuildRequires:		ghostscript-devel
%endif

# Scientific Linux 6: have choice for libjpeg-devel needed by qt-devel: libjpeg-devel libjpeg-turbo-devel
%if 0%{?scientificlinux_version} == 600
BuildRequires:		libjpeg-turbo-devel
%endif

# RHEL, CentOS and Scientific Linux 7 have too old cmake version (use prebuild)
%if 0%{?rhel_version} == 700 || 0%{?scientificlinux_version} == 700 || 0%{?centos_version} == 700
%else
BuildRequires:		cmake
%endif

# wxWidgets BuildRequires
%if 0%{?fedora_version}
%if 0%{?fedora_version} >= 30
BuildRequires:		wxGTK3-devel
%else
BuildRequires:		wxGTK-devel
%endif
%endif
%if 0%{?suse_version}
BuildRequires:		wxWidgets-devel
%endif

# Qt5 BuildRequires for Fedora
%if 0%{?fedora_version} >= 23 || 0%{?centos_version} >= 800
%define qmake_qt5 qmake-qt5
BuildRequires:		qt5-qtbase-devel
%endif


%description
GR, a universal framework for visualization applications

%prep
%setup -n gr-%{fixedversion}
mkdir -p %{THIRDPARTY}
mkdir -p %{THIRDPARTY_SRC}
%{__cp} %{SOURCE1} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE2} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE3} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE4} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE5} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE6} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE7} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE8} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE9} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE10} %{THIRDPARTY_SRC}
%{__cp} %{SOURCE11} %{THIRDPARTY_SRC}
%{__tar} -C %{THIRDPARTY} -xf %{SOURCE7}

%build
# RHEL, CentOS and Scientific Linux 7 have too old cmake version (use prebuild)
%if 0%{?rhel_version} == 700 || 0%{?scientificlinux_version} == 700 || 0%{?centos_version} == 700
export PATH=`pwd`/%{THIRDPARTY}/cmake-3.6.3-Linux-x86_64/bin:$PATH
%endif
make -C 3rdparty GRDIR=%{grdir} DIR=`pwd`/%{THIRDPARTY}
make -C 3rdparty extras GRDIR=%{grdir} DIR=`pwd`/%{THIRDPARTY}
%if 0%{?__jcns}
export CC=/usr/local/gcc/bin/gcc74
export CXX=/usr/local/gcc/bin/g++74
%endif
make GRDIR=%{grdir} \
     EXTRA_CFLAGS=-I`pwd`/%{THIRDPARTY_INC} \
     EXTRA_CXXFLAGS=-I`pwd`/%{THIRDPARTY_INC} \
     EXTRA_LDFLAGS=-L`pwd`/%{THIRDPARTY_LIB} \
     THIRDPARTYDIR=`pwd`/%{THIRDPARTY} \
     %{?qmake_qt4:QT4_QMAKE=%{qmake_qt4}} \
     %{?qmake_qt5:QT5_QMAKE=%{qmake_qt5}}

%install
%{__install} -m 755 -d $RPM_BUILD_ROOT%{grdir}
make install GRDIR=%{grdir} THIRDPARTYDIR=`pwd`/%{THIRDPARTY} DESTDIR=${RPM_BUILD_ROOT}

%clean
make realclean

%files
%defattr(-,root,root)
%{grdir}
