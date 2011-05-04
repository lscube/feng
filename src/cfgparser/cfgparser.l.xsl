<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
                xmlns:exslt="http://exslt.org/common"
		xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output omit-xml-declaration="yes" method="text" />

  <xsl:variable name="newline"><xsl:text><![CDATA[
]]></xsl:text></xsl:variable>

  <xsl:variable name="tokenval">abcdefghijklmnopqrstuvwxyz-</xsl:variable>
  <xsl:variable name="tokennam">ABCDEFGHIJKLMNOPQRSTUVWXYZ_</xsl:variable>
  <xsl:variable name="setname" >abcdefghijklmnopqrstuvwxyz_</xsl:variable>

  <xsl:template match="/">
    <xsl:text><![CDATA[
/* Automatically-generated code, do not modify! */

%option noyywrap yylineno

%{

#include <stdbool.h>
#include <string.h>
#include <glib.h>

#include "cfgparser.parse.h"

/* Avoid outputting what is read */
#define ECHO

]]></xsl:text>

    <xsl:for-each select="/cfgparser/section[@unique='true']">
      <xsl:text>static int </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_seen = 0;</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:text><![CDATA[
%}

%%

"{" |
"}" |
";"		return yytext[0];

"#".*		;

]]></xsl:text>

    <xsl:for-each select="/cfgparser/section">
      <xsl:value-of select="@name" />
      <xsl:text>		</xsl:text>
      <xsl:text>return </xsl:text>

      <xsl:if test="@unique = 'true'">
        <xsl:text>( </xsl:text>
        <xsl:value-of select="translate(@name, $tokenval, $setname)" />
        <xsl:text>_seen++ > 0 ) ? </xsl:text>
        <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
        <xsl:text>_INVALID : </xsl:text>
      </xsl:if>

      <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
      <xsl:text>_TOKEN;</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:for-each select="/cfgparser/section/value">
      <xsl:value-of select="@name" />
      <xsl:text>		</xsl:text>
      <xsl:text>return </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
      <xsl:text>_TOKEN;</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:text><![CDATA[

"true" |
"on" |
"t"		yylval.boolean=true; return BOOLEAN;

"false" |
"off" |
"f"		yylval.boolean=false; return BOOLEAN;

\"[^\"]+\"	yylval.string=g_strndup(yytext+1, strlen(yytext)-2); return STRING;
-?[0-9]+		yylval.integer=atoi(yytext); return INTEGER;

[ \t\n]+                  /* ignore whitespace */;
%%
]]></xsl:text>
  </xsl:template>
</xsl:stylesheet>
