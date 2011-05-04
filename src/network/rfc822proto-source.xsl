<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="lowercase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
  <xsl:variable name="uppercase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

  <xsl:template match="/rfc822proto">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

#include "config.h"
#include <glib.h>
#include "rfc822proto.h"

/* When building PIC code, avoid tables here */
#if __PIC__ && !defined(AVOID_TABLES)
# define AVOID_TABLES 1
#endif

]]></xsl:text>

    <xsl:text><![CDATA[
#if !AVOID_TABLES
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

      <xsl:text>static const char *</xsl:text>
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

    return responses[code];
}
]]></xsl:text>

    </xsl:for-each>

    <xsl:text><![CDATA[
const char *rfc822_response_reason(RFC822_Protocol proto, int code)
{
    static const char *const responses[1000] = {
]]></xsl:text>

    <xsl:for-each select="./response">
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

    if ( responses[code] != NULL )
        return responses[code];

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

#else /* AVOID_TABLES */

const char *rfc822_header_to_string(RFC822_Header hdr)
{
    switch(hdr) {
]]></xsl:text>

    <xsl:for-each select="//supportedheader[not(.=preceding::supportedheader)]">
      <xsl:text>        case </xsl:text>
      <xsl:value-of select="../@name" />
      <xsl:text>_Header_</xsl:text>
      <xsl:value-of select="translate(., '-', '_')" />
      <xsl:text>: return "</xsl:text>
      <xsl:value-of select="." />
      <xsl:text><![CDATA[";
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
        default: g_assert_not_reached();
    }

    return NULL;
}

]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:variable name="loname" select="translate(@name, $uppercase, $lowercase)" />

      <xsl:text>static const char *</xsl:text>
      <xsl:value-of select="$loname" />
      <xsl:text><![CDATA[_response_reason(int code) {
    switch(code) {
]]></xsl:text>

      <xsl:for-each select="response">
	<xsl:text>        case </xsl:text>
	<xsl:value-of select="@code" />
	<xsl:text>: return "</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA[";
]]></xsl:text>
      </xsl:for-each>

    <xsl:text><![CDATA[
        default: g_assert_not_reached();
    }

    return NULL;
}

]]></xsl:text>

    </xsl:for-each>

    <xsl:text><![CDATA[
const char *rfc822_response_reason(RFC822_Protocol proto, int code)
{
    switch(code) {
]]></xsl:text>

    <xsl:for-each select="./response">
      <xsl:text>        case </xsl:text>
      <xsl:value-of select="@code" />
      <xsl:text>: return "</xsl:text>
      <xsl:value-of select="." />
      <xsl:text><![CDATA[";
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
    };

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
#endif /* AVOID_TABLES */
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


