<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="lowercase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
  <xsl:variable name="uppercase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>
  <xsl:variable name="newline"><xsl:text><![CDATA[
]]></xsl:text></xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

#ifndef RFC822PROTO_CONSTANTS_H__
#define RFC822PROTO_CONSTANTS_H__

typedef enum RFC822_Protocol {
    RFC822_Protocol_Invalid,
    RFC822_Protocol_Unsupported,
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
} RFC822_Protocol;

typedef enum RFC822_Header {
    RFC822_Header__Invalid,
    RFC822_Header__Unsupported,
]]></xsl:text>

    <xsl:for-each select="//supportedheader[not(.=preceding::supportedheader)]">
      <xsl:text>    RFC822_Header_</xsl:text>
      <xsl:value-of select="translate(., '-', '_')" />
      <xsl:text><![CDATA[,
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
} RFC822_Header;

]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:variable name="proto" select="@name" />

      <xsl:text>typedef enum </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method {
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method__Invalid,
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method__Unsupported,
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
      <xsl:text>_Header {</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Header__Invalid,</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Header__Unsupported,</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:for-each select="supportedheader">
	<xsl:text>    </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text> = RFC822_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text>,</xsl:text>
	<xsl:value-of select="$newline" />
      </xsl:for-each>

      <xsl:value-of select="$newline" />

      <xsl:text>} </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Header;</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:value-of select="$newline" />

      <xsl:value-of select="$newline" />
      <xsl:value-of select="$newline" />

      <xsl:text>typedef enum </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_ReponseCode {</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:for-each select="/rfc822proto/response|response">
	<xsl:text>    </xsl:text>
	<xsl:value-of select="$proto" />
	<xsl:text>_</xsl:text>

	<xsl:choose>
	  <xsl:when test="@enum">
	    <xsl:value-of select="@enum" />
	  </xsl:when>
	  <xsl:otherwise>
	      <xsl:value-of select="translate(., ' ', '')" />
	  </xsl:otherwise>
	</xsl:choose>

	<xsl:text> = </xsl:text>
	<xsl:value-of select="@code" />
	<xsl:text>,</xsl:text>
	<xsl:value-of select="$newline" />
      </xsl:for-each>

      <xsl:text>} </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_ResponseCode;</xsl:text>
      <xsl:value-of select="$newline" />
      <xsl:value-of select="$newline" />

    </xsl:for-each>

    <xsl:text><![CDATA[
const char *rfc822_header_to_string(RFC822_Header hdr);
const char *rfc822_response_reason(RFC822_Protocol proto, int code);
const char *rfc822_proto_to_string(RFC822_Protocol proto);

#endif /* RFC822PROTO_CONSTANTS_H__ */
]]></xsl:text>
  </xsl:template>
</xsl:stylesheet>
