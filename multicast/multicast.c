/***************************************************************************
                          main.c  -  description
                             -------------------
    begin                : Wed Apr 28 10:53:08 CEST 2004
    copyright            : (C) 2004 by Federico Ridolfo
    email                : federico.ridolfo@polito.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <fenice/xmlparser.h>
#include <fenice/xmllexer.h>
#include <fenice/multicast.h>
/* Global usage: xmlaction(char *xmlfile) that returns an int (EXIT_SUCCESS OR EXIT_FAILURE)*/

static void xml_parser_verify_and_start (xml_node_t *node,char now[35],char source[50]) {

  xml_property_t *p;
  xml_node_t     *n;
  int             l;
  char time[35]="";
  char com[100]="";
  l = strlen (node->name);
  p = node->props;
  while (p) {
    if(strcmp(p->name,"SOURCE")==0){
        strcpy(source,p->value);
        }
    if(strcmp(p->name,"DATE")==0){
        strcpy(time,p->value);
        strcat(time,"-");
      }
    if(strcmp(p->name,"HOUR")==0){
        strcat(time,p->value);
        strcat(time,"-");
      }

    if(strcmp(p->name,"MIN")==0){
        strcat(time,p->value);
      }
    p = p->next;
  }
  n = node->child;
  
  if(strcmp(time,now)==0 /*&& not started yet*/){ 
    /* it's the right time to start*/
    printf("\nStartin multicast session: time=%s now=%s source=%s\n",time,now,source);
    sleep(60);   
    /* i have to start the rtp streams*/ 
    //if(fork()==0){
    //}
  }
  
  while (n) {
    xml_parser_verify_and_start (n,now,source);
    n = n->next;
  }
 }


char *getMounth(char nm[5]){
  if(strcmp(nm,"Jan")==0)
    return "01";
  if(strcmp(nm,"Feb")==0)
    return "02";
  if(strcmp(nm,"Mar")==0)
    return "03";
  if(strcmp(nm,"Apr")==0)
    return "04";
  if(strcmp(nm,"May")==0)
    return "05";
  if(strcmp(nm,"Jun")==0)
    return "06";
  if(strcmp(nm,"Jul")==0)
    return "07";
  if(strcmp(nm,"Aug")==0)
    return "08";
  if(strcmp(nm,"Sep")==0)
    return "09";
  if(strcmp(nm,"Oct")==0)
    return "10";
  if(strcmp(nm,"Nov")==0)
    return "11";
  if(strcmp(nm,"Dec")==0)
    return "12";
 return "none";  
}
  
int multicast(char *argv_s) /*argv_s is the xmlfile name*/ 
{
  char            myxmlfile[100];
  char           *buf = NULL;
  int             buf_size = 0;
  int             buf_used = 0;
  int             len;
  int             fd;
  xml_node_t     *xml_tree;
  int             result;
  struct tm *now;
  char *date;
  char hour[5],min[5],year[5],mounth[5],daynum[5],dayname[5],sec[5];
  char now_confronto[35];
  char source[50]="";
  // Fake timespec for fake nanosleep. See below.
  struct timespec ts = { 0, 0};

   strcpy(myxmlfile,argv_s);
    /* read file to memory. Warning: dumb code, but hopefuly ok since reference file is small */
   if((fd=open(myxmlfile,O_RDONLY))<0){
     printf("\nWarning: there is a problem to open the xml file\n");
     return EXIT_FAILURE;
   }

  
  do {
    buf_size += 1024;
    buf =(char*) realloc(buf, buf_size+1);

    len = read((int)fd, &buf[buf_used], buf_size-buf_used);

    if( len > 0 )
      buf_used += len;

    /* 50k of reference file? no way. something must be wrong */
    if( buf_used > 50*1024 )
      break;
  } while( len > 0 );

  close(fd); /*close the xml file*/
  
  if(buf_used)
    buf[buf_used] = '\0';

  xml_parser_init(buf, buf_used, XML_PARSER_CASE_INSENSITIVE);
  if((result = xml_parser_build_tree(&xml_tree)) != XML_PARSER_OK){
     printf("\nERROR IN XML_PARSER_BUILD_TREE\n");
      return EXIT_FAILURE;
   }
  
  while(1){  
  /*get the current time*/
    time(&now);
    date=ctime(&now);
    sscanf(date,"%s %s %s %2s:%2s:%2s %s",dayname,mounth,daynum,hour,min,sec,year);
    if(strlen(daynum)<2){
    	strcpy(now_confronto,"0");
	strcat(now_confronto,daynum);
	strcpy(daynum,now_confronto);
    }
	    
    strcpy(now_confronto,year);
    strcat(now_confronto,"-");

    strcat(now_confronto,getMounth(mounth));
    strcat(now_confronto,"-");

    strcat(now_confronto,daynum);
    strcat(now_confronto,"-");
    strcat(now_confronto,hour);
    strcat(now_confronto,"-");
    strcat(now_confronto,min);  
         
    /*end get the current time*/
    
   xml_parser_verify_and_start (xml_tree,now_confronto,source);
   nanosleep(&ts, NULL);
   //xml_parser_free_tree(xml_tree);
   
}//end while(1)

  return EXIT_SUCCESS;
}
