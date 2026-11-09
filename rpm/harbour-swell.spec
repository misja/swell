Name:       harbour-swell

Summary:    Controller for BluOS players
Version:    0.6.0
Release:    1
License:    MIT
URL:        https://github.com/misja/swell
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   qml(Amber.Mpris)
# The player's own pages in the app (spec 006); the package names, as Harbour allows.
Requires:   sailfish-components-webview-qt5
Requires:   sailfish-components-webview-qt5-popups
# WebView.qml imports Sailfish.WebView.Pickers, which the package does not require; see issue #32.
Requires:   sailfish-components-webview-qt5-pickers
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  desktop-file-utils
# Sailfish WebView < 1.8 asks apps to link against qtmozembed; see issue #32.
BuildRequires:  pkgconfig(qt5embedwidget)

%description
Controls BluOS players: playback and volume, presets and inputs, the play
queue, browsing and search, grouping and firmware updates. Not affiliated with or endorsed by Lenbrook Industries. BluOS is a
trademark of Lenbrook Industries Limited.

%if 0%{?_chum}
Title: Swell
Type: desktop-application
DeveloperName: Misja Hoebe
Categories:
 - AudioVideo
 - Audio
 - Player
Custom:
  Repo: https://github.com/misja/swell
PackageIcon: https://raw.githubusercontent.com/misja/swell/main/icons/harbour-swell.svg
Links:
  Homepage: https://github.com/misja/swell
  Bugtracker: https://github.com/misja/swell/issues
AIRating: V
AINote: Written mostly by AI coding agents; the maintainer specifies and reviews every change and tests each feature on a device.
%endif


%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 

%make_build


%install
%qmake5_install
# The platform's brp-strip removes only debug info (strip -g); the Harbour
# validator warns unless the symbol table is gone too. Chum builds a
# debuginfo package from the unstripped binary instead.
%if !0%{?_chum}
%{__strip} %{buildroot}%{_bindir}/%{name}
%endif


desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications             \
   %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
