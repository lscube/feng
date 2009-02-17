/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

#ifndef FN_DESCRIPTION_H
#define FN_DESCRIPTION_H

#include "demuxer.h"

/**
 * @file description.h
 * @brief utility wrappers to get description items
 */

static inline time_t r_descr_last_change(ResourceDescr *r_descr)
{
    return r_descr ? r_descr->last_change : 0;
}

static inline char *r_descr_mrl(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->mrl : NULL;
}

static inline char *r_descr_twin(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info && *r_descr->info->twin) ?
            r_descr->info->twin : NULL;
}

static inline char *r_descr_multicast(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info && *r_descr->info->multicast) ?
            r_descr->info->multicast : NULL;
}

static inline char *r_descr_ttl(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info && *r_descr->info->ttl) ?
            r_descr->info->ttl : NULL;
}

static inline char *r_descr_name(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->name : NULL;
}

static inline char *r_descr_description(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->description : NULL;
}

static inline char *r_descr_descrURI(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->descrURI : NULL;
}

static inline char *r_descr_email(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->email : NULL;
}

static inline char *r_descr_phone(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->phone : NULL;
}

static inline double r_descr_time(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->duration : 0.0;
}

static inline sdp_field_list r_descr_sdp_private(ResourceDescr *r_descr)
{
    return (r_descr && r_descr->info) ? r_descr->info->sdp_private : NULL;
}

static inline char *m_descr_name(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info) ? m_descr->info->name : NULL;
}

static inline float m_descr_frame_rate(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ?
            m_descr->properties->frame_rate : 0;
}

static inline MediaType m_descr_type(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ?
            m_descr->properties->media_type : MP_undef;
}

static inline sdp_field_list m_descr_sdp_private(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ?
            m_descr->properties->sdp_private : NULL;
}

static inline int m_descr_rtp_port(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info) ? m_descr->info->rtp_port : 0;
}

static inline int m_descr_rtp_pt(MediaDescr *m_descr)
{
    return (m_descr && m_descr->properties) ?
            m_descr->properties->payload_type : 0;
}

static inline char *m_descr_commons_deed(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info && *m_descr->info->commons_deed) ?
            m_descr->info->commons_deed : NULL;
}

static inline char *m_descr_rdf_page(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info && *m_descr->info->rdf_page) ?
            m_descr->info->rdf_page : NULL;
}

static inline char *m_descr_title(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info && *m_descr->info->title) ?
            m_descr->info->title : NULL;
}

static inline char *m_descr_author(MediaDescr *m_descr)
{
    return (m_descr && m_descr->info && *m_descr->info->author) ?
            m_descr->info->author : NULL;
}

#endif // FN_DESCRIPTION_H
