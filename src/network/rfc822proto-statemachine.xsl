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

%%{
    machine RFC822Proto;

    include common "common.rl";

    RFC822_Generic_Protocol =
        ([A-Z]+ . '/' . digit . "." . digit)
        % { protocol_code = RFC822_Protocol_Unsupported; };

    RFC822_Generic_Method = unreserved+;
]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:text>    RFC822_</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Versions = '</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[/' > ( protocol_code, 2 ) . (]]></xsl:text>
      <xsl:value-of select="$newline" />
      <xsl:for-each select="supportedversion">
	<xsl:text>            '</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA[' > ( protocol_code, 3 )
		% { protocol_code = RFC822_Protocol_]]></xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:value-of select="translate(., '.', '')" />
	<xsl:text>; } |</xsl:text>
      <xsl:value-of select="$newline" />
      </xsl:for-each>
      <xsl:text><![CDATA[            (digit . "." . digit) > ( protocol_code, 1 )
		% { protocol_code = RFC822_Protocol_]]></xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_UnsupportedVersion; }</xsl:text>
      <xsl:value-of select="$newline" />
      <xsl:text>        );</xsl:text>
      <xsl:value-of select="$newline" />
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:value-of select="$newline" />
    <xsl:text>    RFC822_Protocol = (</xsl:text>
    <xsl:value-of select="$newline" />

    <xsl:for-each select="//supportedproto">
      <xsl:text>        RFC822_</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Versions |</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:text><![CDATA[        RFC822_Generic_Protocol
    );

]]></xsl:text>
    <xsl:for-each select="//supportedproto">
      <xsl:variable name="loname" select="translate(@name, $uppercase, $lowercase)" />

      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method = (
         RFC822_Generic_Method % (]]></xsl:text>
      <xsl:value-of select="$loname" />
      <xsl:text>_method_code, 0)  % { </xsl:text>
      <xsl:value-of select="$loname" />
      <xsl:text>_method_code = </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Method__Unsupported; }</xsl:text>
      <xsl:for-each select="supportedmethod">
      <xsl:text><![CDATA[ |
        ']]></xsl:text>
	<xsl:value-of select="." />
	<xsl:text>' > (</xsl:text>
	<xsl:value-of select="$loname" />
	<xsl:text>_method_code, 1) % { </xsl:text>
	<xsl:value-of select="$loname" />
	<xsl:text>_method_code = </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Method_</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>; }</xsl:text>
      </xsl:for-each>

      <xsl:text><![CDATA[
    );
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header_Name =
        (unreserved+) % ( header_code, 0 ) % { header_code = ]]></xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header__Unsupported; } | (
]]></xsl:text>

      <xsl:for-each select="supportedheader">
	<xsl:text>            </xsl:text>
	<xsl:text>'</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA['i > ( header_code, 1 ) % { header_code = ]]></xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text>; }</xsl:text>
	<xsl:if test="position()!=last()">
	  <xsl:text> |</xsl:text>
	</xsl:if>
	<xsl:text><![CDATA[
]]></xsl:text>
      </xsl:for-each>

      <xsl:text><![CDATA[        );

]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
}%%

]]></xsl:text>
  </xsl:template>
</xsl:stylesheet>


