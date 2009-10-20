<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="newline"><xsl:text><![CDATA[
]]></xsl:text></xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[

/* Automatically-generated code, do not modify! */

%%{
    machine RFC822RequestLine;

    include RFC822Proto "rfc822proto-statemachine.rl";

    action set_s {
        s = p;
    }

    action end_method {
        method_str = s;
        method_len = p-s;
    }

    action end_protocol {
        protocol_str = s;
        protocol_len = p-s;
    }

    action end_object {
        object_str = s;
        object_len = p-s;
    }
]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:value-of select="$newline" />
      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_SL_Method := ( </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Method . SP) @ { p = pp; fret; };</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:value-of select="$newline" />

    <xsl:text><![CDATA[
    RFC822_Request_Line := (
        RFC822_Generic_Method > set_s % end_method . SP .
            (print*) > set_s % end_object . SP .
            ( RFC822_Generic_Protocol > set_s % end_protocol ]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:text> | </xsl:text>
      <xsl:value-of select="$newline" />
      <xsl:text>              RFC822_</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Versions % { pp = p; p = method_str-1; fcall </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_SL_Method; }</xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
            )  . CRLF ) %to { fbreak; };

}%%

]]></xsl:text>
  </xsl:template>
</xsl:stylesheet>
