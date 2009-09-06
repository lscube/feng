<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="lowercase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
  <xsl:variable name="uppercase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

#ifndef RFC822PROTO_H__
#define RFC822PROTO_H__

typedef enum RFC822_Protocol {
    RFC822_Protocol_Invalid = -2,
    RFC822_Protocol_Unsupported = -1,
]]></xsl:text>
    
    <xsl:for-each select="//supportedproto">
      <xsl:for-each select="supportedversion">
	<xsl:text>    RFC822_Protocol_</xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:value-of select="translate(., '.', '')" />
	<xsl:text><![CDATA[,
]]></xsl:text>
      </xsl:for-each>

      <xsl:text>    RFC822_Protocol_</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_UnsupportedVersion,
]]></xsl:text>
    </xsl:for-each>
    
    <xsl:text><![CDATA[
} RFC822_Protocols;

]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:text>typedef enum </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method {
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method__Invalid = -1,
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method__Unsupported = -1,
]]></xsl:text>
      
      <xsl:for-each select="supportedmethod">
	<xsl:text>    </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Method_</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA[,
]]></xsl:text>
      </xsl:for-each>

    <xsl:text><![CDATA[
} ]]></xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text><![CDATA[_Method;

]]></xsl:text>

      <xsl:text>typedef enum </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header {
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header__Invalid = -1,
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header__Unsupported = -1,
]]></xsl:text>

      <xsl:for-each select="supportedheader">
	<xsl:text>    </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text><![CDATA[,
]]></xsl:text>
      </xsl:for-each>

    <xsl:text><![CDATA[
} ]]></xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text><![CDATA[_Header;

]]></xsl:text>

    <xsl:text>const char *</xsl:text>
    <xsl:value-of select="translate(@name, $uppercase, $lowercase)" />
    <xsl:text>_header_to_string(</xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>_Header hdr);</xsl:text>

    </xsl:for-each>

    <xsl:text><![CDATA[

#endif /* RFC822PROTO_H__ */
]]></xsl:text>
  </xsl:template>
</xsl:stylesheet>


