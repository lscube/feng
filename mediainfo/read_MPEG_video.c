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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediainfo.h>
#include <fenice/prefs.h>


int read_MPEG_video (media_entry *me, uint8 *data, uint32 *data_size, double *mtime, int *recallme)   /* reads MPEG-1,2 Video */
{
        char thefile[255];
        uint32 num_bytes;
        char *vsh1_1;
        #ifdef MPEG2VSHE
        char *vsh2_1;
	#endif
	int count,flag,extra_bytes,seq_head_pres=0;
        unsigned char *data_aux;                				/* variables used to read next timestamp */
        unsigned data_size_aux=0;               				/* using the same reading functions and */
	double next_timestamp;
	static_MPEG_video *s=NULL;

        if (!(me->flags & ME_FD)) {
                if (!(me->flags & ME_FILENAME)) {
                        return ERR_INPUT_PARAM;
                }
                strcpy(thefile,prefs_get_serv_root());
                strcat(thefile,me->filename);
                me->fd=open(thefile,O_RDONLY);
                if (me->fd==-1) {
                        return ERR_NOT_FOUND;
                }
                me->flags|=ME_FD;
		s = (static_MPEG_video *) calloc (1, sizeof(static_MPEG_video));
		me->stat = (void *) s;
		s->final_byte=0x00;
		s->std=TO_PROBE;
		s->fragmented=0;
        } else 
		s = (static_MPEG_video *) me->stat;

        num_bytes = (me->description).byte_per_pckt;

        *data_size=0;
        count=0;
        flag = 1;                                                                 /* At this point it should be right to find the nearest lower frame */
                                                                                  /* computing it from the value of mtime */
        lseek(me->fd,s->data_total,SEEK_SET);                                        /* and starting the reading from this; */
                                                                                  /* feature not yet implemented, usefull for random access*/
        //*data=(unsigned char *)calloc(1,65000);
        //if (*data==NULL) {
	//	
        //       return ERR_ALLOC;
        //}

        data_aux=(unsigned char *)calloc(1,100);
        if (data_aux==NULL) {
		
                return ERR_ALLOC;
        }

        if (s->data_total == 0) {
                probe_standard(data,data_size,me->fd,&s->std);
                lseek(me->fd,0,SEEK_SET);
        }

        if (s->std == MPEG_2) {
        	#ifdef MPEG2VSHE
                *data_size = 8;
                #else
                *data_size = 4;
		#endif                               				/* bytes for the video specific header */
        } else {
                *data_size = 4;
        }

        if (!s->fragmented) {
        	if ( next_start_code(data,data_size,me->fd) == -1) {
			close(me->fd);
			
			return ERR_EOF;
		}

        	read(me->fd,&s->final_byte,1);
        	data[*data_size]=s->final_byte;
        	*data_size+=1;
                                                        // Start of ES header reading

        	if (s->final_byte == 0xb3) {
                	read_seq_head(data,data_size,me->fd,&s->final_byte,s->std);
                	seq_head_pres=1;
        	}
        	while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {          	// read sequence_extension
                	count=next_start_code(data,data_size,me->fd);           	// and user_data (MPEG-2 only)
                	read(me->fd,&s->final_byte,1);
                	data[*data_size]=s->final_byte;
                	*data_size+=1;
        	}
         	if ((num_bytes!=0) && (*data_size>num_bytes)) {
			count+=4;
			*data_size-=count;
			*recallme=1;
                        								// finds subsequent s->picture to read timestamp
			if (s->final_byte == 0xb8) {
                		read_gop_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->hours,&s->minutes,&s->seconds,&s->picture,s->std);
        		}

        		while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {          // read extension
                		next_start_code(data_aux,&data_size_aux,me->fd);       // and user_data (MPEG-2 only)
                		read(me->fd,&s->final_byte,1);
                		(data_aux)[data_size_aux]=s->final_byte;
                		data_size_aux+=1;
        		}

        		if (s->final_byte == 0x00) {
                		read_picture_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1,s->std);
                		if (s->std == MPEG_2) {
                        		read_picture_coding_ext(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->vsh2);
                		}
        		}

      			s->data_total+=*data_size;
                 	if (s->std==MPEG_2) {
			  	#ifdef MPEG2VSHE
                		s->data_total-=8;
                  		s->vsh1.t=1;
                		#else
                		s->data_total-=4;
                   		s->vsh1.t=0;
				#endif
                	} else {
                 		s->data_total-=4;
                   		s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=0;
                 	s->vsh1.e=0;
                	vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                 	#ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        vsh2_1 = (char *)(&s->vsh2);
                        data[4] = vsh2_1[3];
                        data[5] = vsh2_1[2];
                        data[6] = vsh2_1[1];
                        data[7] = vsh2_1[0];
                	}
			#endif
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
			
                 	return ERR_NOERROR;
         	}

        	if (s->final_byte == 0xb8) {
                	count+=read_gop_head(data,data_size,me->fd,&s->final_byte,&s->hours,&s->minutes,&s->seconds,&s->picture,s->std);
        	}

                if ((num_bytes!=0) && (*data_size>num_bytes)) {
			count+=4;
			*data_size-=count;
			*recallme=1;
                        								// finds subsequent s->picture to read timestamp

        		while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {          // read extension
                		next_start_code(data_aux,&data_size_aux,me->fd);       // and user_data (MPEG-2 only)
                		read(me->fd,&s->final_byte,1);
                		(data_aux)[data_size_aux]=s->final_byte;
                		data_size_aux+=1;
        		}

        		if (s->final_byte == 0x00) {
                		read_picture_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1,s->std);
                		if (s->std == MPEG_2) {
                        		read_picture_coding_ext(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->vsh2);
                		}
        		}

      			s->data_total+=*data_size;
                 	if (s->std==MPEG_2) {
                  	#ifdef MPEG2VSHE
                	s->data_total-=8;
                  	s->vsh1.t=1;
                	#else
                	s->data_total-=4;
                   	s->vsh1.t=0;
			#endif
                	} else {
                 		s->data_total-=4;
                   		s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=0;
                 	s->vsh1.e=0;
                	vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                	#ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        vsh2_1 = (char *)(&s->vsh2);
                        data[4] = vsh2_1[3];
                        data[5] = vsh2_1[2];
                        data[6] = vsh2_1[1];
                        data[7] = vsh2_1[0];
                	}
			#endif
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
			
                 	return ERR_NOERROR;
         	}

        	while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {          	// read extension
                	next_start_code(data,data_size,me->fd);                 	// and user_data (MPEG-2 only)
                	read(me->fd,&s->final_byte,1);
                	data[*data_size]=s->final_byte;
                	*data_size+=1;
        	}

         	if ((num_bytes!=0) && (*data_size>num_bytes)) {
			count+=4;
			*data_size-=count;
			*recallme=1;
                        								// finds subsequent s->picture to read timestamp

        		if (s->final_byte == 0x00) {
                		read_picture_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1,s->std);
                		if (s->std == MPEG_2) {
                        		read_picture_coding_ext(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->vsh2);
                		}
        		}

      			s->data_total+=*data_size;
                 	if (s->std==MPEG_2) {
                  		#ifdef MPEG2VSHE
                		s->data_total-=8;
                  		s->vsh1.t=1;
                		#else
                		s->data_total-=4;
                 		s->vsh1.t=0;
				#endif
                	} else {
                 		s->data_total-=4;
                   		s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=0;
                 	s->vsh1.e=0;
                	vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                	#ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        	vsh2_1 = (char *)(&s->vsh2);
                        	data[4] = vsh2_1[3];
                        	data[5] = vsh2_1[2];
                        	data[6] = vsh2_1[1];
                        	data[7] = vsh2_1[0];
                	}
			#endif
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
			
                 	return ERR_NOERROR;
         	}

        	if (s->final_byte == 0x00) {
                	count+=read_picture_head(data,data_size,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1,s->std);
                 	if ((num_bytes!=0) && (*data_size>num_bytes)) {
				count+=4;
				*data_size-=count;
				*recallme=1;

      				s->data_total+=*data_size;
                 		if (s->std==MPEG_2) {
                  			#ifdef MPEG2VSHE
                			s->data_total-=8;
                  			s->vsh1.t=1;
                			#else
                			s->data_total-=4;
                 			s->vsh1.t=0;
					#endif
                		} else {
                 			s->data_total-=4;
                   			s->vsh1.t=0;
                		}
                		s->vsh1.mbz=0;
                		s->vsh1.an=0;
                		s->vsh1.n=0;
                		if (seq_head_pres) {
                        		s->vsh1.s=1;
                		} else {
                        		s->vsh1.s=0;
                		}
                		s->vsh1.b=0;
                 		s->vsh1.e=0;
                		vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                		data[0] = vsh1_1[3];
                		data[1] = vsh1_1[2];
                		data[2] = vsh1_1[1];
                		data[3] = vsh1_1[0];
                		#ifdef MPEG2VSHE
                		if (s->std == MPEG_2) {
                        		vsh2_1 = (char *)(&s->vsh2);
                  			data[4] = vsh2_1[3];
                        		data[5] = vsh2_1[2];
                        		data[6] = vsh2_1[1];
                        		data[7] = vsh2_1[0];
                		}
				#endif
                		*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
				
                 		return ERR_NOERROR;
         		}

                	if (s->std == MPEG_2) {
                        	count+=read_picture_coding_ext(data,data_size,me->fd,&s->final_byte,&s->vsh2);
                	}
        	}

		if ((num_bytes!=0) && (*data_size>num_bytes)) {
			count+=4;
			*data_size-=count;
			*recallme=1;

      			s->data_total+=*data_size;
                 	if (s->std==MPEG_2) {
                  	#ifdef MPEG2VSHE
                		s->data_total-=8;
                  		s->vsh1.t=1;
                		#else
                		s->data_total-=4;
                 		s->vsh1.t=0;
			#endif
                	} else {
                 		s->data_total-=4;
                   		s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=0;
                 	s->vsh1.e=0;
                	vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                	#ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        	vsh2_1 = (char *)(&s->vsh2);
                        	data[4] = vsh2_1[3];
                        	data[5] = vsh2_1[2];
                        	data[6] = vsh2_1[1];
                        	data[7] = vsh2_1[0];
                	}
			#endif
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
			
                 	return ERR_NOERROR;
         	}

        	while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {          	// read extension
                	next_start_code(data,data_size,me->fd);                    	// and user_data (MPEG-2 only)
                	read(me->fd,&s->final_byte,1);
         		data[*data_size]=s->final_byte;
                	*data_size+=1;
        	}

                if ((num_bytes!=0) && (*data_size>num_bytes)) {
			count+=4;
			*data_size-=count;
			*recallme=1;

      			s->data_total+=*data_size;
                 	if (s->std==MPEG_2) {
                  	#ifdef MPEG2VSHE
                		s->data_total-=8;
                  		s->vsh1.t=1;
                		#else
                		s->data_total-=4;
                 		s->vsh1.t=0;
			#endif
                	} else {
                 		s->data_total-=4;
                   		s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=0;
                 	s->vsh1.e=0;
                	vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                        #ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        	vsh2_1 = (char *)(&s->vsh2);
                        	data[4] = vsh2_1[3];
                        	data[5] = vsh2_1[2];
                        	data[6] = vsh2_1[1];
                        	data[7] = vsh2_1[0];
                	}
			#endif
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
			
                 	return ERR_NOERROR;
         	}


        	if (s->final_byte == 0xb7) {
			*recallme=0;
			s->data_total+=*data_size;
			
			return ERR_NOERROR;
		}

                					// Start of slice reading




        	if (num_bytes != 0) {                                            	/* there could be more than 1 slice per packet */
                	while ( (((*data_size-4) <= num_bytes) && (*recallme == 1))) {    	/* reads slices until num_bytes is reached or next frame is starting*/
                        	count = read_slice(data,data_size,me->fd,&s->final_byte);
                        	next_start_code(data,data_size,me->fd);
                        	read(me->fd,&s->final_byte,1);
                        	data[*data_size]=s->final_byte;
                        	*data_size+=1;
                        	if ( ((s->final_byte > 0xAF)||(s->final_byte==0x00)) && ((*data_size-4) <= num_bytes)) {
                                	*recallme = 0;
                                	flag = 0;
                        	}
                	}
                	if (flag) {      						/* if num_bytes has been exceded */
                        	extra_bytes=*data_size-num_bytes;
                        	lseek(me->fd,-extra_bytes,SEEK_CUR);     		/* removes extra bytes */
                        	*data_size-=extra_bytes;
                        	s->fragmented=1;
                         	*recallme=1;
                	} else {                        				/*  next start-code isn't a slice start code */
                        	lseek(me->fd,-4,SEEK_CUR);
                        	*data_size-=4;
                	}
                	s->data_total+=*data_size;
                 	if (s->std==MPEG_2) {
                  	#ifdef MPEG2VSHE
                		s->data_total-=8;
                  		s->vsh1.t=1;
                		#else
                		s->data_total-=4;
                 		s->vsh1.t=0;
			#endif
                	} else {
                 		s->data_total-=4;
                   		s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=1;
                 	if (!s->fragmented) {
                 		s->vsh1.e=1;
                   	} else {
				s->vsh1.e=0;
                    	}
                	vsh1_1 = (char *)(&s->vsh1);  					/* to see the struct as a set of bytes */
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                	#ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        	vsh2_1 = (char *)(&s->vsh2);
                        	data[4] = vsh2_1[3];
                        	data[5] = vsh2_1[2];
                        	data[6] = vsh2_1[1];
                        	data[7] = vsh2_1[0];
                	}
			#endif
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
                	if (s->final_byte == 0xb7) {
                        	next_start_code(data,data_size,me->fd);
                        	read(me->fd,&s->final_byte,1);
                        	data[*data_size]=s->final_byte;
                        	*data_size+=1;
                       	 	s->data_total+=4;
                	}

                	if (*recallme == 0)	{					/* reads next time-stamp to compute the value of delta_mtime */

                 		lseek(me->fd,4,SEEK_CUR);

                		if (s->final_byte == 0xb3) {
                		read_seq_head(data_aux,&data_size_aux,me->fd,&s->final_byte,s->std);
        			}
      		  		while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
         				next_start_code(data_aux,&data_size_aux,me->fd);
               	 			read(me->fd,&s->final_byte,1);
                			(data_aux)[data_size_aux]=s->final_byte;
                			data_size_aux+=1;
        			}
        			if (s->final_byte == 0xb8) {
                			read_gop_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->hours,&s->minutes,&s->seconds,&s->picture,s->std);
        			}

        			while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
                			next_start_code(data_aux,&data_size_aux,me->fd);
                			read(me->fd,&s->final_byte,1);
                			(data_aux)[data_size_aux]=s->final_byte;
                			data_size_aux+=1;
        			}

        			if (s->final_byte == 0x00) {
                			read_picture_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1_aux,s->std);
                			if (s->std == MPEG_2) {
                        			read_picture_coding_ext(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->vsh2_aux);
                			}
        			}

        			while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
                			next_start_code(data_aux,&data_size_aux,me->fd);
                			read(me->fd,&s->final_byte,1);
                			(data_aux)[data_size_aux]=s->final_byte;
                			data_size_aux+=1;
        			}
           										/* computes delta_mtime */
         			next_timestamp = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
          			me->description.delta_mtime = next_timestamp - *mtime;

                	}

			
                	return ERR_NOERROR;
        	} else {                                            			/* 1 slice per packet */
         		read_slice(data,data_size,me->fd,&s->final_byte);
                	s->data_total+=*data_size;
                	if (s->std==MPEG_2) {
                 		#ifdef MPEG2VSHE
                		s->data_total-=8;
                  		s->vsh1.t=1;
                		#else
                		s->data_total-=4;
                 		s->vsh1.t=0;
				#endif
                	} else {
                        	s->data_total-=4;
                        	s->vsh1.t=0;
                	}
                	s->vsh1.mbz=0;
                	s->vsh1.an=0;
                	s->vsh1.n=0;
                	if (seq_head_pres) {
                        	s->vsh1.s=1;
                	} else {
                        	s->vsh1.s=0;
                	}
                	s->vsh1.b=s->vsh1.e=1;
                	vsh1_1 = (char *)(&s->vsh1);
                	data[0] = vsh1_1[3];
                	data[1] = vsh1_1[2];
                	data[2] = vsh1_1[1];
                	data[3] = vsh1_1[0];
                	#ifdef MPEG2VSHE
                	if (s->std == MPEG_2) {
                        	vsh2_1 = (char *)(&s->vsh2);
                        	data[4] = vsh2_1[3];
                        	data[5] = vsh2_1[2];
                        	data[6] = vsh2_1[1];
                        	data[7] = vsh2_1[0];
                	}
			#endif
               	 	if ((s->final_byte > 0xAF)||(s->final_byte==0x00)) {
                        	*recallme = 0;
                	} else {
                        	*recallme = 1;
                	}
                	*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) +  (s->picture*40);
                	if (s->final_byte == 0xb7) {
                        	next_start_code(data,data_size,me->fd);
                        	read(me->fd,&s->final_byte,1);
                       	 	data[*data_size]=s->final_byte;
                        	*data_size+=1;
                        	s->data_total+=4;
                	}

                	if (*recallme == 0)	{					/* reads next time-stamp to compute the value of delta_mtime */

                 		lseek(me->fd,4,SEEK_CUR);

                		if (s->final_byte == 0xb3) {
                		read_seq_head(data_aux,&data_size_aux,me->fd,&s->final_byte,s->std);
        			}
      		  		while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
         				next_start_code(data_aux,&data_size_aux,me->fd);
               	 			read(me->fd,&s->final_byte,1);
                			(data_aux)[data_size_aux]=s->final_byte;
                			data_size_aux+=1;
        			}
        			if (s->final_byte == 0xb8) {
                			read_gop_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->hours,&s->minutes,&s->seconds,&s->picture,s->std);
        			}

        			while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
                			next_start_code(data_aux,&data_size_aux,me->fd);
                			read(me->fd,&s->final_byte,1);
                			(data_aux)[data_size_aux]=s->final_byte;
                			data_size_aux+=1;
        			}

        			if (s->final_byte == 0x00) {
                			read_picture_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1_aux,s->std);
                			if (s->std == MPEG_2) {
                        			read_picture_coding_ext(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->vsh2_aux);
                			}
        			}

        			while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
                			next_start_code(data_aux,&data_size_aux,me->fd);
                			read(me->fd,&s->final_byte,1);
                			(data_aux)[data_size_aux]=s->final_byte;
                			data_size_aux+=1;
        			}
           										/* computes delta_mtime */
         			next_timestamp = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
          			me->description.delta_mtime = next_timestamp - *mtime;

             		}

			
                	return ERR_NOERROR;
        	}
  	} else {                                   					/* if previous slice had been s->fragmented */
	        count=read_slice(data,data_size,me->fd,&s->final_byte);
	        next_start_code(data,data_size,me->fd);
  		read(me->fd,&s->final_byte,1);
    		data[*data_size]=s->final_byte;
      		*data_size+=1;
          	if ((s->final_byte > 0xAF)||(s->final_byte==0x00)) {
           		*recallme = 0;
           	} else {
            		*recallme = 1;
           	}

            	*data_size-=4;
	   	if(*data_size>num_bytes){
			extra_bytes=*data_size-num_bytes;
			lseek(me->fd,-extra_bytes,SEEK_CUR);     			/* removes last slice inserted */
   			*data_size-=extra_bytes;
      			s->fragmented=1;
         		*recallme=1;
	    	} else {
			s->fragmented=0;
	     	}
		*mtime = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);

             	s->data_total+=*data_size;

             	if (s->std==MPEG_2) {
              		#ifdef MPEG2VSHE
                	s->data_total-=8;
                  	s->vsh1.t=1;
                	#else
                	s->data_total-=4;
                 	s->vsh1.t=0;
			#endif
                } else {
                	s->data_total-=4;
                 	s->vsh1.t=0;
                }

              	s->vsh1.mbz=0;
               	s->vsh1.an=0;
                s->vsh1.n=0;
               	if (seq_head_pres) {
                	s->vsh1.s=1;
                } else {
                	s->vsh1.s=0;
                }
                s->vsh1.b=0;
                if (!s->fragmented) {
                	s->vsh1.e=1;
                 } else {
			s->vsh1.e=0;
   		}
                vsh1_1 = (char *)(&s->vsh1);
                data[0] = vsh1_1[3];
                data[1] = vsh1_1[2];
                data[2] = vsh1_1[1];
                data[3] = vsh1_1[0];
                #ifdef MPEG2VSHE
                if (s->std == MPEG_2) {
                       	vsh2_1 = (char *)(&s->vsh2);
                       	data[4] = vsh2_1[3];
                       	data[5] = vsh2_1[2];
                       	data[6] = vsh2_1[1];
                       	data[7] = vsh2_1[0];
                }
		#endif

            	if (*recallme == 0)	{						/* reads next time-stamp to compute the value of delta_mtime */
             		//lseek(me->fd,4,SEEK_CUR);
               		if (s->final_byte == 0xb3) {
                		read_seq_head(data_aux,&data_size_aux,me->fd,&s->final_byte,s->std);
        		}
      		  	while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
         			next_start_code(data_aux,&data_size_aux,me->fd);
            			read(me->fd,&s->final_byte,1);
               			(data_aux)[data_size_aux]=s->final_byte;
                		data_size_aux+=1;
        		}
        		if (s->final_byte == 0xb8) {
          			read_gop_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->hours,&s->minutes,&s->seconds,&s->picture,s->std);
        		}

        		while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
          			next_start_code(data_aux,&data_size_aux,me->fd);
             			read(me->fd,&s->final_byte,1);
                		(data_aux)[data_size_aux]=s->final_byte;
                		data_size_aux+=1;
        		}

        		if (s->final_byte == 0x00) {
          			read_picture_head(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->temp_ref,&s->vsh1_aux,s->std);
             			if (s->std == MPEG_2) {
                			read_picture_coding_ext(data_aux,&data_size_aux,me->fd,&s->final_byte,&s->vsh2_aux);
                		}
        		}

        		while ((s->final_byte == 0xb5) || (s->final_byte == 0xb2)) {
          			next_start_code(data_aux,&data_size_aux,me->fd);
             			read(me->fd,&s->final_byte,1);
                		(data_aux)[data_size_aux]=s->final_byte;
                		data_size_aux+=1;
        		}
          										/* computes delta_mtime */
         		next_timestamp = (s->hours * 3.6e6) + (s->minutes * 6e4) + (s->seconds * 1000) +  (s->temp_ref*40) + (s->picture*40);
          		me->description.delta_mtime = next_timestamp - *mtime;
            	}

		
            	return ERR_NOERROR;

  	}
}


