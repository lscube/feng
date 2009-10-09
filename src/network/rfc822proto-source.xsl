<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="lowercase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
  <xsl:variable name="uppercase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

#include <glib.h>
#include "rfc822proto.h"

const char *rfc822_header_to_string(RFC822_Header hdr)
{
    static const char *const header_names[] = {
]]></xsl:text>

    <xsl:for-each select="//supportedheader[not(.=preceding::supportedheader)]">
      <xsl:text>        [</xsl:text>
      <xsl:value-of select="../@name" />
      <xsl:text>_Header_</xsl:text>
      <xsl:value-of select="translate(., '-', '_')" />
      <xsl:text>] = "</xsl:text>
      <xsl:value-of select="." />
      <xsl:text><![CDATA[",
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
        NULL
    };

    g_assert(hdr > 0 &&
             (size_t)hdr < sizeof(header_names)/sizeof(header_names[0]));

    return header_names[hdr];
}

]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:variable name="loname" select="translate(@name, $uppercase, $lowercase)" />

      <xsl:text>const char *</xsl:text>
      <xsl:value-of select="$loname" />
      <xsl:text><![CDATA[_response_reason(int code) {
    static const char *const responses[1000] = {
]]></xsl:text>

      <xsl:for-each select="response">
	<xsl:text>        [</xsl:text>
	<xsl:value-of select="@code" />
	<xsl:text>] = "</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA[",
]]></xsl:text>
      </xsl:for-each>

      <xsl:text><![CDATA[
        [999] = NULL
    };

    g_assert(100 <= code && code <= 999);
    return responses[code];
}
]]></xsl:text>

    </xsl:for-each>

    <xsl:text><![CDATA[
const char *rfc822_response_reason(RFC822_Protocol proto, int code)
{
    switch(proto)
    {
]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:for-each select="supportedversion">
	<xsl:text>    case RFC822_Protocol_</xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:value-of select="translate(., '.', '')" />
	<xsl:text><![CDATA[:
]]></xsl:text>
      </xsl:for-each>
      <xsl:text>    case RFC822_Protocol_</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_UnsupportedVersion:
]]></xsl:text>
      <xsl:text>        return </xsl:text>
      <xsl:value-of select="translate(@name, $uppercase, $lowercase)" />
      <xsl:text>_response_reason(code);</xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
    };

    g_assert_not_reached();
    return NULL;
}
]]></xsl:text>

    <xsl:text><![CDATA[
const char *rfc822_proto_to_string(RFC822_Protocol proto)
{
    switch(proto)
    {
]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:for-each select="supportedversion">
	<xsl:text>    case RFC822_Protocol_</xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:value-of select="translate(., '.', '')" />
	<xsl:text><![CDATA[:
        return "]]></xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>/</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA[";
]]></xsl:text>
      </xsl:for-each>
      <xsl:text>    case RFC822_Protocol_</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_UnsupportedVersion:
        return "]]></xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>/</xsl:text>
      <xsl:value-of select="supportedversion[last()]" />
	<xsl:text><![CDATA[";
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
    };

    g_assert_not_reached();
    return NULL;
}
]]></xsl:text>

  </xsl:template>
</xsl:stylesheet>


