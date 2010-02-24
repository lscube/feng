%%{
    machine common;

    action mark {mark = fpc;}

	BIT = "0" | "1";
	CHAR = 0x01..0x7f;
	CR = "\r";
	LF = "\n";
	CRLF = CR* LF;
	CTL = 0x00..0x1f | 0x7f;
	DQUOTE = "\"";
	HTAB = "\t";
	SP = " ";
	WSP = SP | HTAB;
	LWSP = ( WSP | ( CRLF WSP ) )*;
	OCTET = 0x00..0xff;
	VCHAR = 0x21..0x7e;
	scheme = alpha ( alpha | digit | "+" | "-" | "." )*;
	unreserved = alpha | digit | "-" | "." | "_" | "~";
	pct_encoded = "%" xdigit xdigit;
	sub_delims = "!" | "$" | "&" | "'" | "(" | ")" | "*" | "+" | "," | ";" | "=";
	userinfo = ( unreserved | pct_encoded | sub_delims | ":" )*;
	h16 = xdigit{1,4};
	dec_octet = digit | ( 0x31..0x39 digit ) | ( "1" digit{2} ) | ( "2" 0x30..0x34 digit ) | ( "25" 0x30..0x35 );
	IPv4address = dec_octet "." dec_octet "." dec_octet "." dec_octet;
	ls32 = ( h16 ":" h16 ) | IPv4address;
	IPv6address = ( ( h16 ":" ){6} ls32 ) | ( "::" ( h16 ":" ){5} ls32 ) | ( h16? "::" ( h16 ":" ){4} ls32 ) | ( ( ( h16 ":" )? h16 )? "::" ( h16 ":" ){3} ls32 ) | ( ( ( h16 ":" ){,2} h16 )? "::" ( h16 ":" ){2} ls32 ) | ( ( ( h16 ":" ){,3} h16 )? "::" h16 ":" ls32 ) | ( ( ( h16 ":" ){,4} h16 )? "::" ls32 ) | ( ( ( h16 ":" ){,5} h16 )? "::" h16 ) | ( ( ( h16 ":" ){,6} h16 )? "::" );
	IPvFuture = "v"i xdigit+ "." ( unreserved | sub_delims | ":" )+;
	IP_literal = "[" ( IPv6address | IPvFuture ) "]";
}%%
