Name:       qmlfirmataplugin

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}


Summary:    QML Plugin for Firmata
Version:    0.0.1
Release:    1
Group:      System/Libraries
License:    GPL
URL:        https://github.com/kimmoli/qfirmata
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)

%description
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build

%qtc_qmake5 VERSION=%{version}

%qtc_make %{?_smp_mflags}


%install
rm -rf %{buildroot}

%qmake5_install


%files
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/kimmoli/firmata/libqmlfirmataplugin.so
%{_libdir}/qt5/qml/org/kimmoli/firmata/qmldir
# >> files
# << files
