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
#include <getopt.h>
#include <fenice/command_environment.h>

void usage(){
	fprintf(stderr,"fenice [--config-file | -c <config_file>]\n\n");
}

uint32 command_environment(int argc, char **argv){


    static const char	short_options[]= "r:p:c:";
				        //"m:a:f:n:b:z:T:B:q:o:S:I:r:M:4:2:Q:X:D:g:G:v:V:F:N:tpdsZHOcCPK:E:R:";

    int n;
    uint32 nerr = 0;/*number of error*/
    uint32 config_file_not_present=1;
    uint32 flag=0;/*0 to show help*/
//#ifdef HAVE_GETOPT_LONG
    static struct option long_options[]={
        { "config-file",           1, 0, 'c' },
        { "rtsp-port",             1, 0, 'p' },
        { "root-dir",              1, 0, 'r' },
        { "help",                  0, 0, '?' },
        { 0,                       0, 0, 0 }
    };


    while( (n=getopt_long(argc,argv,short_options,long_options, NULL)) != -1 )
//#else
//    while( (n=getopt(argc,argv,short_options)) != -1)
//#endif
	{
		flag=1;
		switch(n) {
     		   	case 0 :                /* Flag setting handled by getopt-long */
           		break;

        		case 'c' :
				 // = atoi(optarg);
				 prefs_init(optarg);
				 config_file_not_present=0;
				/* prefs_init() loads root directory, port, hostname and domain name on
   				a static variable prefs */
			break;
			case 'p':

			break;
			case 'r' :
				
			break;

			case ':' :
				fprintf(stderr, "Missing parameter to option!");
			break;
			case '?':
				flag=0;
				nerr++;
			break;
			default:
				nerr++;
		}
	}
    if(!flag){
    	nerr++;
	usage();
    }
    else if(config_file_not_present)
	    prefs_init(NULL);
    return nerr;
}
