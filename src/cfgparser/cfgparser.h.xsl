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

#ifndef __CFGPARSER_H__
#define __CFGPARSER_H__

#include <stdbool.h>
#include <glib.h>
#include <stdio.h>
#include <stdint.h>

extern int yyparse();
extern void yyerror(const char *fmt, ...);
extern FILE *yyin;
extern int yylineno;

]]></xsl:text>

    <xsl:apply-templates />

    <xsl:text><![CDATA[
#endif
]]></xsl:text>
  </xsl:template>

  <xsl:template match="section">
    <xsl:text>typedef struct cfg_</xsl:text>
    <xsl:value-of select="translate(@name, $tokenval, $setname)" />
    <xsl:text>_t {</xsl:text>
    <xsl:apply-templates />
    <xsl:text>} cfg_</xsl:text>
    <xsl:value-of select="translate(@name, $tokenval, $setname)" />
    <xsl:text>_t;</xsl:text>
    <xsl:value-of select="$newline" />
    <xsl:value-of select="$newline" />
    <xsl:text>  bool cfg_</xsl:text>
    <xsl:value-of select="translate(@name, $tokenval, $setname)" />
    <xsl:text>_callback(cfg_</xsl:text>
    <xsl:value-of select="translate(@name, $tokenval, $setname)" />
    <xsl:text>_t *section);</xsl:text>
  </xsl:template>

  <xsl:template match="value">
    <xsl:text>  </xsl:text>
    <xsl:choose>
      <xsl:when test="@type='string'">char *</xsl:when>
      <xsl:when test="@type='boolean'">bool</xsl:when>
      <xsl:when test="@type='integer'">int32_t</xsl:when>
      <xsl:when test="@type='uinteger'">uint32_t</xsl:when>
      <xsl:when test="@type='stringlist'">GList *</xsl:when>
    </xsl:choose>
    <xsl:text> </xsl:text>
    <xsl:value-of select="translate(@name, $tokenval, $setname)" />
    <xsl:text>;</xsl:text>
  </xsl:template>

  <xsl:template match="raw">
    <xsl:value-of select="." />
  </xsl:template>
</xsl:stylesheet>
