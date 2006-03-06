/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 * 
 *  - (LS)³ Team			<team@streaming.polito.it>	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#ifndef _STUNH	
#define _STUNH

#include <fenice/types.h>

#define STUN_MAX_MESSAGE_SIZE 2048

/*---- STUN HEADER SECTION ----*/ 

/* Stun Header:
 * _____________________________________________________
 * | Message Type (16 bits) | Message Length (16 bits) |
 * -----------------------------------------------------
 * |						       |
 * |                                                   |
 * |            Transaction ID (128 bits)              |
 * |                                                   |
 * -----------------------------------------------------
 *
 */

struct STUN_HEADER{
	uint16 msgtype;	
	uint32 msglen;
	uint128 transactionID;
};

/* Define Message Types values */
/*Binding*/
#define BINDING_REQUEST 0x0001
#define BINDING_RESPONSE 0x0101
#define BINDING_ERROR_RESPONSE 0x0111
/*Shared Secret (not enabled in the first implementation*/
#define SHARED_SECRET_REQUEST 0x0002
#define SHARED_SECRET_RESPONSE 0x0102
#define SHARED_SECRET_ERROR_RESPONSE 0x0002

/*---- END STUN HEADER SECTION ----*/


/*---- STUN PAYLOAD SECTION ----*/ 

/* Stun Payload:
 * _________________________________________________________
 * | Attribute Type (16 bits) | Attribute Length (16 bits) |
 * ---------------------------------------------------------
 * |                                                       |
 * |           Attribute Value (Variable Length)           |
 * |                                                       |
 * ---------------------------------------------------------
 *
 */

/*Define Attribute Types*/
#define MAPPED_ADDRESS 0x0001 
#define RESPONSE_ADDRESS 0x0002
#define CHANGE_REQUEST 0x0003
#define SOURCE_ADDRESS 0x0004
#define CHANGED_ADDRESS 0x0005
#define USERNAME 0x0006
#define PASSWORD 0x0007
#define MESSAGE_INTEGRITY 0x0008
#define ERROR_CODE 0x0009
#define UNKNOWN_ATTRIBUTES 0x000A
#define REFLECTED_FROM 0x000B

struct STUN_ATR_HEADER{
      uint16 type;
      uint16 length;
};

/*define structs for different Atribute*/
/*TODO: one struct for one Atribute*/

/*---- END STUN PAYLOAD SECTION ----*/

/*Packet*/
/*
 typedef struct STUN_PKT {
	struct STUN_HEADER stun_hdr;
	struct STUN_ATR_HEADER stun_atr_hdr;
	//... structs for all atributes ...  
 } OMS_STUN_PKT;
 *
 * */
/*a mask variable to set end unset the different Attributes*/
/*
typedef struct STUN_PKT_DEV {
	OMS_STUN_PKT stun_pkt;
	uint16 set_atr_mask; //only 11 bits are needed
}
*/

#endif //__STUNH
