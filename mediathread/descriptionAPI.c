/* * 
 *  This file is part of Feng
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
 * */

/* here we have some utils to handle the ResourceDescr and MediaDescr structire
 * types in order to simplify its usage to the mediathread library user */

#include <string.h>
#include <fenice/demuxer.h>

/* --- ResourceInfo wrapper functions --- */

inline time_t r_descr_last_change(ResourceDescr *r_descr)
{
    return r_descr ? r_descr->last_change : 0;
}

inline char *r_descr_mrl(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->mrl : NULL;
}

inline char *r_descr_twin(ResourceDescr *r_descr)
{
    // use this if 'twin' become a char pointer
    // return (r_descr && r_descr->info) ? r_descr->info->twin : NULL;
    return (r_descr && r_descr->info && *r_descr->info->twin) ? r_descr->info->twin : NULL;
}

inline char *r_descr_multicast(ResourceDescr *r_descr)
{
    // use this if 'multicast' become a char pointer
    // return (r_descr && r_descr->info) ? r_descr->info->multicast : NULL;
    return (r_descr && r_descr->info && *r_descr->info->multicast) ? r_descr->info->multicast : NULL;
}

inline char *r_descr_ttl(ResourceDescr *r_descr)
{
    // use this if 'ttl' become a char pointer
    // return (r_descr && r_descr->info) ? r_descr->info->ttl : NULL;
    return (r_descr && r_descr->info && *r_descr->info->ttl) ? r_descr->info->ttl : NULL;
}

inline char *r_descr_name(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->name : NULL;
}

inline char *r_descr_description(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->description : NULL;
}

inline char *r_descr_descrURI(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->descrURI : NULL;
}

inline char *r_descr_email(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->email : NULL;
}

inline char *r_descr_phone(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->phone : NULL;
}

inline double r_descr_time(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->duration : 0.0;
}

inline sdp_field_list r_descr_sdp_private(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->sdp_private : NULL;
}

/*! This function creates an array of MediaDescrList containing media descriptions.
 * This array is returned to function caller.
 * Each element of the array is a MediaDescrList that contain all the media of the same type with the same name.
 * All the elements of each list can be included together in the sdp description in a single m= block.
 * @param r_descr Resource description that contains all the media
 * @param m_descrs this is a return parameter. It will contain the MediaDescrList array
 * @return the dimension of the array or an interger < 0 if an error occurred.
 * */
MediaDescrListArray r_descr_get_media(ResourceDescr *r_descr)
{
    MediaDescrListArray new_m_descrs;
    MediaDescrList m_descr_list, m_descr;
    guint i;
    gboolean found;
    
    new_m_descrs = g_ptr_array_sized_new(
                        g_list_position(r_descr->media,
                                        g_list_last(r_descr->media))+1);
    
    for (m_descr = g_list_first(r_descr->media);
         m_descr;
         m_descr = g_list_next(m_descr)) {
        found = FALSE;
        for (i = 0; i < new_m_descrs->len; ++i) {
            m_descr_list = g_ptr_array_index(new_m_descrs, i);
            if ( (m_descr_type(MEDIA_DESCR(m_descr)) ==
                  m_descr_type(MEDIA_DESCR(m_descr_list))) &&
                 !strcmp(m_descr_name(MEDIA_DESCR(m_descr)),
                         m_descr_name(MEDIA_DESCR(m_descr_list))) ) {
                found = TRUE;
                break;
            }
        }
        if (found) {
            m_descr_list = g_ptr_array_index(new_m_descrs, i);
            m_descr_list = g_list_prepend(m_descr_list, MEDIA_DESCR(m_descr));
            new_m_descrs->pdata[i]=m_descr_list;
//            printf("*!* %u, %d\n", new_m_descrs->len, m_descr_type(MEDIA_DESCR(m_descr)));
        } else {
            m_descr_list = g_list_prepend(NULL, MEDIA_DESCR(m_descr));
            g_ptr_array_add(new_m_descrs, m_descr_list);
//            printf("*?* %u, %d\n", new_m_descrs->len, m_descr_type(MEDIA_DESCR(m_descr)));
        }
    }
    
    for (i = 0; i < new_m_descrs->len; ++i) {
            m_descr_list = g_ptr_array_index(new_m_descrs, i);
            m_descr_list = g_list_reverse(m_descr_list);
    }
        
    return new_m_descrs;
}

inline char *m_descr_name(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info) ? m_descr->info->name : NULL;
}

inline MediaType m_descr_type(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ? m_descr->properties->media_type : MP_undef;
}

inline sdp_field_list m_descr_sdp_private(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ? m_descr->properties->sdp_private : NULL;
}

inline int m_descr_rtp_port(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info) ? m_descr->info->rtp_port : 0;
}

inline int m_descr_rtp_pt(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ? m_descr->properties->payload_type : 0;
}

inline char *m_descr_commons_deed(MediaDescr *m_descr)
{
    // use this if 'twin' become a char pointer
//    return (m_descr && m_descr->info) ? m_descr->info->commons_deed : NULL;
    return (m_descr && m_descr->info && *m_descr->info->commons_deed) ? m_descr->info->commons_deed : NULL;
}

inline char *m_descr_rdf_page(MediaDescr *m_descr)
{
    // use this if 'twin' become a char pointer
//    return (m_descr && m_descr->info) ? m_descr->info->rdf_page : NULL;
    return (m_descr && m_descr->info && *m_descr->info->rdf_page) ? m_descr->info->rdf_page : NULL;
}

inline char *m_descr_title(MediaDescr *m_descr)
{
    // use this if 'twin' become a char pointer
//    return (m_descr && m_descr->info) ? m_descr->info->title : NULL;
    return (m_descr && m_descr->info && *m_descr->info->title) ? m_descr->info->title : NULL;
}

inline char *m_descr_author(MediaDescr *m_descr)
{
    // use this if 'twin' become a char pointer
//    return (m_descr && m_descr->info) ? m_descr->info->author : NULL;
    return (m_descr && m_descr->info && *m_descr->info->author) ? m_descr->info->author : NULL;
}
