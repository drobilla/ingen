Summary: A realtime modular synthesizer for GNU/Linux audio systems with support Jack and LADSPA or DSSI plugins.
Name: om
Version: 0.3.0pre
Release: 1
License: GPL
Group: Applications/Multimedia
URL: http://www.nongnu.org/om-synth/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires:	liblo jackit libxml2 libalsa2 ladspa libdssi0 libgtkmm2.4_1 libglademm2.4_1 libgnomecanvasmm2.6_1
BuildRequires: liblo-devel libjack0-devel libxml2-devel libalsa2-devel ladspa-devel libdssi0-devel libgtkmm2.4_1-devel libglademm2.4_1-devel libgnomecanvasmm2.6_1-devel

%description
Om is a modular synthesizer for GNU/Linux audio systems with:
Completely OSC controlled engine, (meaning you can control Om over a network, from any number of clients),
LADSPA and DSSI plugin support, A minimal number of special internal "plugins" (MIDI in, audio outputs, transport, etc.), Polyphony, Multiple top-level patches, with subpatching (unlimited depth).
%prep
%setup -q

%build
%configure
make

%install
%{__rm} -rf %{buildroot}
%{__make} install \
	DESTDIR="%{buildroot}"


%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/om
%{_bindir}/om_gtk
%{_bindir}/om_patch_loader
%dir %{_datadir}/om
%{_datadir}/om/patches/*.om
%dir %{_datadir}/om-gtk
%{_datadir}/om-gtk/om_gtk.glade
%{_datadir}/om-gtk/om-icon.png


%doc AUTHORS ChangeLog COPYING NEWS README THANKS


%changelog
* Tue Apr 26 2005 Loki Davison <loki@berlios.de>
- initial package, v0.0.1

