/** 
 * This file is part of Feng 
 *  
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it>  
 * See AUTHORS for more details  
 *   
 * Feng is free software; you can redistribute it and/or   
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 of  
 * the License, or (at your option) any later version. 
 *   
 * Feng is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * General Public License for more details.  
 *  
 * You should have received a copy of the GNU General Public License 
 * along with Feng; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef FN_COMMAND_ENVIRONMENT_H
#define FN_COMMAND_ENVIRONMENT_H

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <fenice/prefs.h>
#include <fenice/server.h>
#include <fenice/fnc_log.h>

int command_environment(feng *srv, int argc, char **argv);
#endif // FN_COMMAND_ENVIRONMENT_H
