 /* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */


#include <stdio.h>
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <config.h>

#include <fenice/fnc_log.h>
#include <fenice/debug.h>

//static char* fnc_name;
static FILE *fd;

void (*fnc_log)(int level, const char *fmt, ...);
#if HAVE_SYSLOG_H
	static void fnc_syslog(int level, const char *fmt, ...);
#endif
	static void fnc_errlog(int level, const char *fmt, ...);

/*Set fnc_log as fprintf on file or syslog or stderr*/
void fnc_log_init(char* name, int out){/*if out == FNC_LOG_OUT => stderr*/
//	fnc_name = name;
	fnc_log=fnc_errlog;

	if(out){
		if((fd=fopen(name,"a+"))==NULL){
#if HAVE_SYSLOG_H
			openlog("fenice ", LOG_PID /*| LOG_PERROR*/ , LOG_DAEMON);
			fnc_log=fnc_syslog;
#else
			fd=stderr;
#endif
		}
	}
	else
		fd=stderr;
	
}

/*Prints on standard error or file*/
static void fnc_errlog(int level, const char *fmt, ...){
	va_list args;
	time_t now;
	char *date;
	int no_print=0;

	time(&now);
	date=ctime(&now);

	date[strlen(date) -1]='\0';

	switch (level) {
		case FNC_LOG_ERR:
			fprintf(fd, "%s Error: ",date);
			break;
		case FNC_LOG_ERR_FATAL:
			fprintf(fd, "%s Fatal Error: ",date);
			break;
		case FNC_LOG_WARN:
			fprintf(fd, "%s Warning: ",date);
			break;
		case FNC_LOG_DEBUG:
#if DEBUG
			fprintf(fd, "Debug: ");
#else
			no_print=1;	 
#endif
			break;
		case FNC_LOG_VERBOSE:
#ifdef VERBOSE
			fprintf(fd, "Verbose Debug: ");
#else
			no_print=1;	 
#endif
			break;
		default:
			/*FNC_LOG_INFO*/
			fprintf(fd, "%s : ",date);
			break;
	}

	if(!no_print){
		va_start(args, fmt);
		vfprintf(fd, fmt, args);
		va_end(args);
		fflush(fd);
	}
}


#if HAVE_SYSLOG_H
static void fnc_syslog(int level, const char *fmt, ...){
	va_list args;
	int l=LOG_ERR;
	int no_print=0;

	switch (level) {
		case FNC_LOG_ERR:
			l=LOG_ERR;
			break;
		case FNC_LOG_ERR_FATAL:
			l=LOG_ERR;
			break;
		case FNC_LOG_WARN:
			l=LOG_WARNING;
			break;
		case FNC_LOG_DEBUG:
#if DEBUG
			l=LOG_DEBUG;
#else
			no_print=1;	 
#endif
			break;
		case FNC_LOG_VERBOSE:
#ifdef VERBOSE
			l=LOG_DEBUG;
#else
			no_print=1;	 
#endif
			break;
		default:
			l=LOG_INFO;
			break;
	}
	
	if(!no_print){
		va_start(args, fmt);
		vsyslog(l, fmt, args);
		va_end(args);
	}
}
#endif


