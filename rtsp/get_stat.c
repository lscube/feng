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

#include <fenice/rtsp.h>

char *get_stat(int err)
{
	struct {
		char *token;
		int code;
	} status[] = {
		{
		"Continue", 100}, {
		"OK", 200}, {
		"Created", 201}, {
		"Accepted", 202}, {
		"Non-Authoritative Information", 203}, {
		"No Content", 204}, {
		"Reset Content", 205}, {
		"Partial Content", 206}, {
		"Multiple Choices", 300}, {
		"Moved Permanently", 301}, {
		"Moved Temporarily", 302}, {
		"Bad Request", 400}, {
		"Unauthorized", 401}, {
		"Payment Required", 402}, {
		"Forbidden", 403}, {
		"Not Found", 404}, {
		"Method Not Allowed", 405}, {
		"Not Acceptable", 406}, {
		"Proxy Authentication Required", 407}, {
		"Request Time-out", 408}, {
		"Conflict", 409}, {
		"Gone", 410}, {
		"Length Required", 411}, {
		"Precondition Failed", 412}, {
		"Request Entity Too Large", 413}, {
		"Request-URI Too Large", 414}, {
		"Unsupported Media Type", 415}, {
		"Bad Extension", 420}, {
		"Invalid Parameter", 450}, {
		"Parameter Not Understood", 451}, {
		"Conference Not Found", 452}, {
		"Not Enough Bandwidth", 453}, {
		"Session Not Found", 454}, {
		"Method Not Valid In This State", 455}, {
		"Header Field Not Valid for Resource", 456}, {
		"Invalid Range", 457}, {
		"Parameter Is Read-Only", 458}, {
		"Unsupported transport", 461}, {
		"Internal Server Error", 500}, {
		"Not Implemented", 501}, {
		"Bad Gateway", 502}, {
		"Service Unavailable", 503}, {
		"Gateway Time-out", 504}, {
		"RTSP Version Not Supported", 505}, {
		"Option not supported", 551}, {
		"Extended Error:", 911}, {
		NULL, -1}
	};
	int i;
	for (i = 0; status[i].code != err && status[i].code != -1; ++i);
	return status[i].token;
}
