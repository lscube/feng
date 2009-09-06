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

#include <glib.h>
#include "rfc822proto.h"

]]></xsl:text>
    
    <xsl:for-each select="//supportedproto">
      <xsl:variable name="proto_lower"
		    select="translate(@name, $uppercase, $lowercase)" />

      <xsl:text>extern </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Method </xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text>_method_str_to_enum(const char*);</xsl:text>

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

      <exslt:document href="{$proto_lower}-method.rl"
		      encoding="UTF-8" method="text">
	<xsl:text><![CDATA[
/* Automatically generated file, do not modify.
 * The next line is used to propely clean up the generated files at
 * distclean time, so please ignore it. Thanks.
 *
 * please_delete_me_on_distclean
 */

#include <string.h>
#include "rfc822proto.h"

]]></xsl:text>

	<xsl:text>%% machine </xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text><![CDATA[_method_tokenizer;

]]></xsl:text>

	<xsl:text><![CDATA[%% include RFC822Proto "rfc822proto-statemachine.rl";
]]></xsl:text>

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
    main := ]]></xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_Method . 0;

    write data noerror;
    write init;
    write exec;
}%%

    return method_code;
}
]]></xsl:text>
      </exslt:document>

      <xsl:text>extern </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>_Header </xsl:text>
      <xsl:value-of select="$proto_lower" />
      <xsl:text>_header_str_to_enum(const char*);</xsl:text>

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

      <exslt:document href="{$proto_lower}-header.rl"
		      encoding="UTF-8" method="text">
	<xsl:text><![CDATA[
/* Automatically generated file, do not modify.
 * The next line is used to propely clean up the generated files at
 * distclean time, so please ignore it. Thanks.
 *
 * please_delete_me_on_distclean
 */

#include <string.h>
#include "rfc822proto.h"

]]></xsl:text>

	<xsl:text>%% machine </xsl:text>
	<xsl:value-of select="$proto_lower" />
	<xsl:text><![CDATA[_header_tokenizer;

]]></xsl:text>

	<xsl:text><![CDATA[%% include RFC822Proto "rfc822proto-statemachine.rl";
]]></xsl:text>

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
    main := ]]></xsl:text>
	<xsl:value-of select="@name" />
	<xsl:text><![CDATA[_Header_Name . 0;

    write data noerror;
    write init;
    write exec;
}%%

    return header_code;
}
]]></xsl:text>
      </exslt:document>

    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>


