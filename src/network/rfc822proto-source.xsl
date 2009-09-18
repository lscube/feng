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

]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:variable name="loname" select="translate(@name, $uppercase, $lowercase)" />

      <xsl:text>const char *</xsl:text>
      <xsl:value-of select="$loname" />
      <xsl:text>_header_to_string(</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header hdr) {
    static const char *const header_names[] = {
]]></xsl:text>

      <xsl:for-each select="supportedheader">
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

    g_assert(100 <= code && code >= 999);
    return responses[code];
}
]]></xsl:text>

    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>


