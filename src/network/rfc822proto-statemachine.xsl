<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="lowercase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
  <xsl:variable name="uppercase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

%%{
]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Methods = (
]]></xsl:text>
      <xsl:for-each select="supportedmethod">
	<xsl:text>        '</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>' % { method_code = </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Method_</xsl:text>
	<xsl:value-of select="." />
	<xsl:text><![CDATA[; } |
]]></xsl:text>
      </xsl:for-each>

      <xsl:text><![CDATA[         unreserved+
    );
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header_Name = (
]]></xsl:text>

      <xsl:for-each select="supportedheader">
	<xsl:text>        '</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>'i % { hdr_code = </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text><![CDATA[; } |
]]></xsl:text>
      </xsl:for-each>

      <xsl:text><![CDATA[         unreserved+
    );
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[
}%%

]]></xsl:text>
  </xsl:template>
</xsl:stylesheet>


