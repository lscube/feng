Summary: A GPL Experimental Streaming Server
Name: fenice
Version: 1.6
Release: 0
Copyright: GPL
Group: Applications/Internet
Vendor: OMSP Team (http://streaming.polito.it)
Packager: OMSP Team (http://streaming.polito.it)
Source: http://streaming.polito.it/download/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-buildroot

%description
Fenice is a streaming server of new conception that allows the generation of
multimedia streams on packet networks based on the IP protocol.
Fenice is compatible with the IETF protocols for the streaming and for real-time data transport.
The main characteristic of Fenice is that it is adaptable to the state of the
network gotten through the technique of the dynamic coding change.
Fenice is the official streaming server for Open Media Streaming project

%prep
%setup -q -n %{name}-%{version}

%build
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/%{name}

%{_localstatedir}/%{name}/avroot/test.sd
%{_localstatedir}/%{name}/avroot/audio.mp3
%{_localstatedir}/%{name}/avroot/video.mpg

%config %{_sysconfdir}/%{name}.conf

%doc README TODO COPYING ChangeLog INSTALL-FAST AUTHORS
%doc docs/en/index*.html
