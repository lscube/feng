/* *
 *  $Id$
 *
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2007 by
 *
 *    - Dario Gallucci    <dario.gallucci@polito.it>
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

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <fenice/utils.h>
#include <fenice/fnc_log.h>

#include <fenice/multicast.h>    /*is_valid_multicast_address */
#include <fenice/rtsp.h>         /*parse_url */
#include <fenice/rtpptdefs.h>    /*payload type definitions */

#include <fenice/demuxer_module.h>

static DemuxerInfo info = {
    "Dynamic Edit List",
    "sd",
    "OMSP Team",
    "",
    "sd"
};


typedef struct __EDL_ITEM_ELEM {
    Resource *r;
    double offset;	//from begin of list
    double begin;	//in seconds
    double end;
    int first_ts;	//init to 0, set to 1 when have begin timestamp
} edl_item_elem;

typedef struct __EDL_PRIV {
    GList *head;
    unsigned int active;
    unsigned int old;
    int move_to_next;
} edl_priv_data;

static int ds_probe(InputStream * i_stream)
{
    char *ext;
    char buffer[80], string[80];
    int version, n;

    // Check filename extension
    if (!((ext = strrchr(i_stream->name, '.')) && (!strcmp(ext, ".ds")))) {
        return RESOURCE_DAMAGED;
    }
    // Read begin of file
    if (istream_read(i_stream, (uint8 *) buffer, 79) <= 0) {
        return RESOURCE_DAMAGED;
    }
    buffer[79] = '\0';
    // Check for "# Version 1"
    if ((n = sscanf(buffer, "#%s%d", string, &version)) != 2) {
        fnc_log(FNC_LOG_DEBUG, "Broken Version string (%d): %s\n", n, buffer);
        return RESOURCE_DAMAGED;
    }
    if (strcmp(string, "Version") || version != 1) {
        fnc_log(FNC_LOG_DEBUG, "Broken string, token=%s, version=%d\n", string, version);
        return RESOURCE_DAMAGED;
    }
    // Move file descriptor to begin
    istream_reset(i_stream);
    return RESOURCE_OK;
}

static inline edl_item_elem *edl_active_res (void *private_data) {
    edl_priv_data *data = ((edl_priv_data *) private_data);
    return (edl_item_elem *) g_list_nth_data(data->head, data->active);
}

static void destroy_list_data(gpointer elem, gpointer unused) {
    edl_item_elem *item = (edl_item_elem *) elem;
    if (item)
    	r_close(item->r);
    g_free(elem);
}

static void change_track_parent(gpointer elem, gpointer new_parent) {
    ((Track *) elem)->parent = new_parent;
}

static double edl_timescaler (Resource * r, double res_time) {
    edl_item_elem *item = edl_active_res(r->edl->private_data);
    if (!item) {
        fnc_log(FNC_LOG_FATAL, "NULL edl item: This shouldn't happen\n");
        return HUGE_VAL;
    }
    if (item->first_ts) {
        item->begin = res_time;
        item->first_ts = 0;
    }
    if (res_time >= item->end) {
        ((edl_priv_data *) r->edl->private_data)->move_to_next = 1;
    }
    return res_time + item->offset - item->begin;
}

static int ds_init(Resource * r)
{
    char line[256], mrl[256];
    double begin = 0.0, end = HUGE_VAL, r_offset = 0.0;
    int n;
    FILE *fd;
    Resource *resource;
    GList *edl_head = NULL;
    edl_item_elem *item;

    fnc_log(FNC_LOG_DEBUG, "EDL init function\n");
    fd = fdopen(r->i_stream->fd, "r");

    do {
        fgets(line, sizeof(line), fd);
        if (feof(fd))
            break;
        n = sscanf(line, "%s%lf%lf", mrl, &begin, &end);
        if (n < 1 || line[0] == '#')
            continue; // skip comments and empty lines

        /* Init Resources required by the EDitList
         * (modifying default behaviour to manipulate timescale)
         * */
        if (!(resource = r_open(prefs_get_serv_root(), mrl))) {
            goto err_alloc;
        }
        if (!(item = g_new0(edl_item_elem, 1))) {
            goto err_alloc;
        }
        // set edl timescaler
        resource->timescaler = edl_timescaler;
        resource->edl = r;
        // set edl properties
        item->r = resource;
        item->begin = begin;
        item->first_ts = 1;
        item->end = end;
        item->offset = r_offset;
        if (resource->info->duration && end > resource->info->duration) {
            r_offset += resource->info->duration - begin;
        } else {
            r_offset += end - begin;
        }
        //seek to begin
        if (begin)
            resource->demuxer->seek(resource, begin);
        edl_head = g_list_prepend(edl_head, item);
        // Use first resource for tracks
        if(!r->tracks) {
            r->tracks = resource->tracks;
        }
	g_list_foreach(resource->tracks, change_track_parent, r);
    } while (!feof(fd));

    if (!(r->private_data = g_new0(edl_priv_data, 1))) {
        goto err_alloc;
    }

    ((edl_priv_data *) r->private_data)->head = g_list_reverse(edl_head);

    return RESOURCE_OK;
err_alloc:
    if (edl_head) {
        g_list_foreach(edl_head, destroy_list_data, NULL);
        g_list_free(edl_head);
        r->private_data = NULL;
    }
    return ERR_ALLOC;
}

/*
static int read_header(Resource * r)
{
    return RESOURCE_OK;
}
*/
static void edl_buffer_switcher(edl_priv_data *data) {
    edl_item_elem *old_item, *new_item;
    GList *p, *q;
    void *tmp;
    if (!data || data->active == data->old)
        return;
    old_item = (edl_item_elem *) g_list_nth_data(data->head, data->old);
    if (!(new_item = (edl_item_elem *) g_list_nth_data(data->head, data->active)))
        return;
    p = old_item->r->tracks;
    q = new_item->r->tracks;
    while (p && q) {
        tmp = TRACK(q)->buffer;
        TRACK(q)->buffer = TRACK(p)->buffer;
        TRACK(p)->buffer = tmp;
        p = g_list_next(p);
        q = g_list_next(q);
    }
    data->old = data->active;
}

static int ds_read_packet(Resource * r)
{
    edl_priv_data *data = ((edl_priv_data *) r->private_data);
    edl_item_elem *item;
    int res;
    fnc_log(FNC_LOG_VERBOSE, "EDL read_packet function\n");
    if (data->move_to_next) {
        data->move_to_next = 0;
        data->active +=1;
        edl_buffer_switcher(data);
        fnc_log(FNC_LOG_DEBUG, "EDL active track: %d\n", data->active);
    }
    if (!(item = edl_active_res (data))) {
        return EOF;
    }
    while ((res = item->r->demuxer->read_packet(item->r)) == EOF) {
        data->active +=1;
        edl_buffer_switcher(data);
        fnc_log(FNC_LOG_DEBUG, "EDL active track: %d\n", data->active);
        if (!(item = edl_active_res (data))) {
            return EOF;
        }
    }
    return res;
}

static int ds_seek(Resource * r, double time_sec)
{
    return RESOURCE_NOT_SEEKABLE;
}

static int ds_uninit(Resource * r)
{
    GList *edl_head = NULL;
    if (r->private_data)
        edl_head = ((edl_priv_data *) r->private_data)->head;
    if (edl_head) {
        g_list_foreach(edl_head, destroy_list_data, NULL);
        g_list_free(edl_head);
        g_free(r->private_data);
        r->private_data = NULL;
    }
    r->tracks = NULL; //Unlink local copy of first resource tracks
    return RESOURCE_OK;
}

FNC_LIB_DEMUXER(ds);

