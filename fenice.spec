Summary: standards-compliant multimedia streaming server
Name: fenice
Version: 1.9
Release: 0
Copyright: GPL
Group: Applications/Internet
Vendor: OMSP Team (http://streaming.polito.it)
Packager: OMSP Team (http://streaming.polito.it)
Source: http://streaming.polito.it/download/%{name}-%{version}.tar.gz

BuildRoot: %{_tmppath}/%{name}-buildroot

%description
Fenice is a multimedia streaming server compliant with the IETF's standards for
real-time streaming of multimedia contents over Internet. Fenice implements
RTSP (Real-Time Streaming Protocol - RFC2326) and RTP/RTCP (Real-Time Transport
Protocol/RTP Control Protocol - RFC3550) with RTP Profile for Audio and Video
Conferences with Minimal Control (RFC3551).

Fenice is the official streaming server for the Open Media Streaming Project
(http://streaming.polito.it) which is is a free/libre project software for the
development of a platform for the streaming of multimedia contents. The
platform is based on the full support of the IETF's standards for the real-time
data transport over IP. The aim of the project is to provide an open solution,
free and interoperable along with the proprietary streaming applications
currently dominant on the market.

%prep
%setup -q -n %{name}-%{version}

%build
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
make

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
rm -rf %{buildroot}%{_datadir}/doc/%{name}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_mandir}/man1/*

/*/%{name}/avroot/test.sd
/*/%{name}/avroot/audio.mp3
/*/%{name}/avroot/video.mpg

%config %{_sysconfdir}/%{name}.conf

%doc NEWS README TODO COPYING ChangeLog INSTALL-FAST AUTHORS
%doc docs/howto/%{name}-howto.html docs/%{name}.1.{ps.gz,pdf,html}
