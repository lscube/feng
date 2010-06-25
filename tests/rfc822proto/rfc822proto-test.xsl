<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:exslt="http://exslt.org/common"
		extension-element-prefixes="exslt">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="lowercase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
  <xsl:variable name="uppercase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

#include <string.h>
#include <glib.h>
#include <stdio.h>
#include "rfc822proto.h"

static RFC822_Protocol rfc822_protocol_str_to_enum(const char *str) {
    RFC822_Protocol protocol_code = RFC822_Protocol_Invalid;
    const char *p = str, *pe = p + strlen(str) + 1;
    int cs;

%%{
    machine rfc822_protocol_tokenizer;

    include RFC822Proto "rfc822proto-statemachine.rl";

    main := RFC822_Protocol . 0;

    write data noerror nofinal;
    write init;
    write exec;
}%%

    return protocol_code;
}

void test_rfc822_tokenizer() {
]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:for-each select="supportedversion">
	<xsl:text>    g_assert_cmpint(rfc822_protocol_str_to_enum("</xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>/</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>"), ==, RFC822_Protocol_</xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:value-of select="translate(., '.', '')" />
	<xsl:text><![CDATA[);
]]></xsl:text>
      </xsl:for-each>

	<xsl:text>    g_assert_cmpint(rfc822_protocol_str_to_enum("</xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text>/9.9"), ==, RFC822_Protocol_</xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_UnsupportedVersion);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(rfc822_protocol_str_to_enum("</xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[/X.Y"), ==, RFC822_Protocol_Invalid);
]]></xsl:text>
    </xsl:for-each>

    <xsl:text><![CDATA[

    g_assert_cmpint(rfc822_protocol_str_to_enum("FOOBAR/1.1"), ==, RFC822_Protocol_Unsupported);
    g_assert_cmpint(rfc822_protocol_str_to_enum("FOOBAR/A.B"), ==, RFC822_Protocol_Invalid);
    g_assert_cmpint(rfc822_protocol_str_to_enum("FoObAr/1.1"), ==, RFC822_Protocol_Invalid);
    g_assert_cmpint(rfc822_protocol_str_to_enum("FoObAr-1.1"), ==, RFC822_Protocol_Invalid);
}

]]></xsl:text>

    <xsl:for-each select="//supportedproto">
      <xsl:variable name="proto_lower"
		    select="translate(@name, $uppercase, $lowercase)" />

      <xsl:text>static </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Method </xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text><![CDATA[_method_str_to_enum(const char *str) {
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Method method_code = </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method__Invalid;
]]></xsl:text>

      <xsl:text><![CDATA[
    const char *p = str, *pe = p + strlen(str) + 1;
    int cs;

%%{
    machine ]]></xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text><![CDATA[_method_tokenizer;

    include RFC822Proto "rfc822proto-statemachine.rl";

    main := ]]></xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Method . 0;

    write data noerror nofinal;
    write init;
    write exec;
}%%

    return method_code;
}
]]></xsl:text>

      <xsl:text><![CDATA[

void test_]]></xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text><![CDATA[_method_tokenizer() {
]]></xsl:text>

      <xsl:for-each select="supportedmethod">
	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_method_str_to_enum("</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Method_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text><![CDATA[);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_method_str_to_enum("</xsl:text>
	<xsl:value-of select="translate(., 'AEIOU', 'XXXXX')" />
	<xsl:text>"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Method__Unsupported</xsl:text>
	<xsl:text><![CDATA[);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_method_str_to_enum("</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>%%TEST"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Method__Invalid</xsl:text>
	<xsl:text><![CDATA[);
]]></xsl:text>
      </xsl:for-each>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_method_str_to_enum("XXTESTXX"), ==, </xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_Method__Unsupported);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_method_str_to_enum("%%TEST%%"), ==, </xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_Method__Invalid);
]]></xsl:text>

      <xsl:text><![CDATA[
}
]]></xsl:text>

      <xsl:text>static </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Header </xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text><![CDATA[_header_str_to_enum(const char *str) {
]]></xsl:text>

      <xsl:text>    </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Header header_code = </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header__Invalid;
]]></xsl:text>

      <xsl:text><![CDATA[
    const char *p = str, *pe = p + strlen(str) + 1;
    int cs;

%%{
    machine ]]></xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text><![CDATA[_header_tokenizer;

    include RFC822Proto "rfc822proto-statemachine.rl";

    main := ]]></xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text><![CDATA[_Header_Name . 0;

    write data noerror nofinal;
    write init;
    write exec;
}%%

    return header_code;
}
]]></xsl:text>

      <xsl:text><![CDATA[

void test_]]></xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text><![CDATA[_header_tokenizer() {
]]></xsl:text>

      <xsl:for-each select="supportedheader">
	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text><![CDATA[);
]]></xsl:text>

	<!-- All-uppercase variant -->
	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("</xsl:text>
	<xsl:value-of select="translate(., $lowercase, $uppercase)" />
	<xsl:text>"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text><![CDATA[);
]]></xsl:text>

	<!-- All-lowercase variant -->
	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("</xsl:text>
	<xsl:value-of select="translate(., $uppercase, $lowercase)" />
	<xsl:text>"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header_</xsl:text>
	<xsl:value-of select="translate(., '-', '_')" />
	<xsl:text><![CDATA[);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("</xsl:text>
	<xsl:value-of select="translate(., 'AEIOUaeiou', 'XXXXXXXXXX')" />
	<xsl:text>"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header__Unsupported</xsl:text>
	<xsl:text><![CDATA[);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("</xsl:text>
	<xsl:value-of select="." />
	<xsl:text>%%TEST"), ==, </xsl:text>
	<xsl:value-of select="../@name" />
	<xsl:text>_Header__Invalid</xsl:text>
	<xsl:text><![CDATA[);
]]></xsl:text>
      </xsl:for-each>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("X-Test-Header"), ==, </xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_Header__Unsupported);
]]></xsl:text>

	<xsl:text>    g_assert_cmpint(</xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text>_header_str_to_enum("%-Test-Header"), ==, </xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_Header__Invalid);
]]></xsl:text>

      <xsl:text><![CDATA[
}
]]></xsl:text>

    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>


