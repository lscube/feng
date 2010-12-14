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

%{

#include <stdbool.h>
#include <glib.h>
#include "cfgparser.h"

extern int yylex (void);

]]></xsl:text>

    <xsl:text>union {</xsl:text>
    <xsl:value-of select="$newline" />
    <xsl:for-each select="/cfgparser/section">
      <xsl:text>cfg_</xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_t </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>;</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>
    <xsl:value-of select="$newline" />
    <xsl:text>} cfg_current;</xsl:text>
    <xsl:value-of select="$newline" />

    <xsl:text><![CDATA[

GList *list;

%}

%union {
  char *string;
  int32_t integer;
  uint32_t uinteger;
  bool boolean;
  GList *stringlist;
}

%token BOOLEAN
%token STRING
%token INTEGER

]]></xsl:text>

    <xsl:for-each select="/cfgparser/section">
      <xsl:text>%token </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
      <xsl:text>_TOKEN "</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>"</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:for-each select="/cfgparser/section[@unique='true']">
      <xsl:text>%token </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
      <xsl:text>_INVALID "</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>(!)"</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:for-each select="/cfgparser/section/value">
      <xsl:text>%token </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
      <xsl:text>_TOKEN "</xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>"</xsl:text>
      <xsl:value-of select="$newline" />
    </xsl:for-each>

    <xsl:text><![CDATA[
%%

commands: | commands command;

STRINGLIST: '{' strings '}' { yylval.stringlist = list; };
UINTEGER: INTEGER { if ( yylval.integer < 0 ) { yyerror("negative values not allowed"); YYERROR; } }

strings: | strings STRING { list = g_list_append(list, yylval.string); };

]]></xsl:text>

    <xsl:text>command:</xsl:text>
    <xsl:for-each select="/cfgparser/section">
      <xsl:text> </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_set</xsl:text>
      <xsl:if test="position()!=last()">
        <xsl:text> |</xsl:text>
      </xsl:if>
    </xsl:for-each>

    <xsl:text>;</xsl:text>
    <xsl:value-of select="$newline" />

    <xsl:value-of select="$newline" />

    <xsl:for-each select="/cfgparser/section">
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_set: </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
      <xsl:text>_TOKEN '{' </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_contents '}' ';'</xsl:text>
      <xsl:text> { if ( !cfg_</xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_callback(&amp;cfg_current.</xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>)) YYERROR; }</xsl:text>
      <xsl:text>;</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_contents: | </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_contents </xsl:text>
      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_content;</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:value-of select="translate(@name, $tokenval, $setname)" />
      <xsl:text>_content: </xsl:text>
      <xsl:for-each select="value">
        <xsl:value-of select="translate(../@name, $tokenval, $setname)" />
        <xsl:text>_</xsl:text>
        <xsl:value-of select="translate(@name, $tokenval, $setname)" />
        <xsl:if test="position()!=last()">
          <xsl:text> | </xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>;</xsl:text>
      <xsl:value-of select="$newline" />

      <xsl:for-each select="value">
        <xsl:value-of select="translate(../@name, $tokenval, $setname)" />
        <xsl:text>_</xsl:text>
        <xsl:value-of select="translate(@name, $tokenval, $setname)" />
        <xsl:text>: </xsl:text>
        <xsl:text> </xsl:text>
        <xsl:value-of select="translate(@name, $tokenval, $tokennam)" />
        <xsl:text>_TOKEN </xsl:text>
        <xsl:value-of select="translate(@type, $tokenval, $tokennam)" />
        <xsl:text> ';'</xsl:text>
        <xsl:text> { cfg_current.</xsl:text>
        <xsl:value-of select="translate(../@name, $tokenval, $setname)" />
        <xsl:text>.</xsl:text>
        <xsl:value-of select="translate(@name, $tokenval, $setname)" />
        <xsl:text> = yylval.</xsl:text>
        <xsl:value-of select="translate(@type, $tokenval, $setname)" />
        <xsl:text>; } </xsl:text>
        <xsl:text>;</xsl:text>
        <xsl:value-of select="$newline" />
      </xsl:for-each>

      <xsl:value-of select="$newline" />
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
