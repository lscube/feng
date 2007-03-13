/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
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
#include <string.h>

#include <fenice/demuxer.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

// global demuxer modules:
extern Demuxer fnc_demuxer_sd;
extern Demuxer fnc_demuxer_ds;
extern Demuxer fnc_demuxer_avf;

// static array containing all the available demuxers:
static Demuxer *demuxers[] = {
    &fnc_demuxer_sd,
    &fnc_demuxer_ds,
    &fnc_demuxer_avf,
    NULL
};

/*! \brief The exclusive tracks.
 * The Media Thread keeps a list of the tracks that can be opened only once.
 * */
static GList *ex_tracks=NULL;
/*! \brief The resource description cache.
 * This list holds the cache of resource descriptions. This way the mediathread
 * can provide the description buffer without opening the resource every time.
 * This will result in a better performance for RTSP DESCRIBE metod response.
 * Descriptions are ordered for access time, so that the last is the less
 * recently addressed media description and will be chosen for removal if cache
 * reaches the limit.
 * */
static GList *descr_cache=NULL;
//! cache size of descriptions (maybe we need to take it from fenice configuration file?)
#define MAX_DESCR_CACHE_SIZE 10

/*! Private functions for exclusive tracks.
 * */
static inline void ex_tracks_save(GList *);
static void ex_track_save(Track *, gpointer *);
static inline void ex_track_remove(Track *);
// static void ex_tracks_free(Track *[], uint32);

static gint cache_cmp(gconstpointer, gconstpointer);
#define r_descr_find(mrl) g_list_find_custom(descr_cache, mrl, cache_cmp)
static void r_descr_cache_update(Resource *);
static ResourceDescr *r_descr_new(Resource *);
static void r_descr_free(ResourceDescr *);

static void resinfo_free(void *);
static void trackinfo_free(void *);

// private functions for specific demuxer
static int find_demuxer(InputStream *);

Resource *r_open(char *root, char *n)
{
    Resource *r;
    int dmx_idx;
    InputStream *i_stream;
    char mrl[255];
    
    snprintf(mrl, sizeof(mrl) - 1, "%s%s%s", root,
         (root[strlen(root) - 1] == '/') ? "" : "/", n);

    if( !(i_stream = istream_open(mrl)) )
        return NULL;

    if ( (dmx_idx = find_demuxer(i_stream))<0 ) {
        fnc_log(FNC_LOG_DEBUG, "[MT] Could not find a valid demuxer"
                                       " for resource %s\n", mrl);
        return NULL;
    }

    fnc_log(FNC_LOG_DEBUG, "[MT] registrered demuxer \"%s\" for resource"
                               "\"%s\"\n", demuxers[dmx_idx]->info->name, mrl);

// allocation of all data structures

    if( !(r = calloc(1, sizeof(Resource))) ) {
        fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
        istream_close(i_stream);
        return NULL;
    }

#if 0 
// we use MObject_new: that will alloc memory and exits
// the program if something goes wrong
    if( !(r->info=calloc(1, sizeof(ResourceInfo))) ) { // init all infos
        fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
        istream_close(i_stream);
        free(r);
        return NULL;
    }
#endif
// we use MObject_new: that will alloc memory and exits the program
// if something goes wrong
    r->info = MObject_new0(ResourceInfo, 1);
    MObject_destructor(r->info, resinfo_free);

/* initialization non needed 'cause we use calloc
    r->private_data=NULL;
    r->demuxer=NULL;
    //initializate tracks??? TODO
    // temporary track initialization:
    r->num_tracks=0;
    */
    r->info->mrl = g_strdup(mrl);
    r->info->name = g_path_get_basename(n);
    r->i_stream = i_stream;
    r->demuxer = demuxers[dmx_idx];

    if (r->demuxer->init(r)) {
        r_close(r);
        return NULL;
    }

    // search for exclusive tracks: should be done track per track?
    ex_tracks_save(r->tracks);
    // update the Description cache
    r_descr_cache_update(r);
    
    return r;
}

void r_close(Resource *r)
{
    if(r) {
        istream_close(r->i_stream);
        MObject_unref(MOBJECT(r->info));
        r->info = NULL;
        if(r->private_data!=NULL)
            free(r->private_data);
        if(r->tracks)
            g_list_foreach(r->tracks, (GFunc)free_track, r);

        free(r);
    }
}

#if 0 // not implemented... use r_descr_...
msg_error get_resource_info(resource_name n, ResourceInfo *r)
{
    //...
    return RESOURCE_OK;
}
#endif // if 0

Selector *r_open_tracks(Resource *r, char *track_name, 
                        Capabilities *capabilities) // RTSP_setup.c uses it !!
{
    Selector *s;
    //Track *tracks[MAX_SEL_TRACKS];
    GList *track, *sel_tracks=NULL;

    /*Capabilities aren't used yet. TODO*/

    for (track=g_list_first(r->tracks); track; track=g_list_next(track))
        if( !strcmp(TRACK(track)->info->name, track_name) ){
            sel_tracks = g_list_prepend(sel_tracks, TRACK(track));
        }
    if (!sel_tracks)
        return NULL;
// now we reverse the order of the list to rebuild the resource tracks order
// Probably this is not so useful: 
// I feel free to remove the instruction sooner or later...

//    sel_tracks = g_list_reverse(sel_tracks);
    
    if((s=(Selector*)malloc(sizeof(Selector)))==NULL) {
        fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
        return NULL;
    }

    s->tracks = sel_tracks;
    s->total = g_list_length(sel_tracks);
    s->default_index = 0;/*TODO*/
    s->selected_index = 0;/*TODO*/
    //...
    return s;
}

inline Track *r_selected_track(Selector *selector) // UNUSED
{
    if (!selector)
        return NULL;

    return g_list_nth_data(selector->tracks, selector->selected_index);
}

void r_close_tracks(Selector *s) // UNUSED
{
    /*see r_close, what i have to do???*/
    // free_track(s->tracks[s->selected_index]);
    /*shawill: probably we must free only selector data*/
    free(s);
}

inline int r_seek(Resource *r, long int time_sec)
{
    return r->demuxer->seek(r,time_sec);
}

int r_changed(ResourceDescr *descr)
{
    GList *m_descr = g_list_first(descr->media);

    if ( mrl_changed(descr->info->mrl, &descr->last_change) )
        return 1;

    for (/* m_descr=descr->media */;
         m_descr && MEDIA_DESCR(m_descr)->info->mrl;
         m_descr=g_list_next(m_descr))
    {
        if (mrl_changed(MEDIA_DESCR(m_descr)->info->mrl,
                        &(MEDIA_DESCR(m_descr)->last_change)))
            return 1;
    }

    return 0;
}

/*
Resource *init_resource(resource_name name)
{
    Resource *r;
    //...
    return r;
}
*/

/*! Add track to resource tree.  This function adds a new track data struct to
 * resource tree. It used by specific demuxer function in order to obtain the
 * struct to fill.
 * \param r pointer to resource.
 * \return pointer to newly allocated track struct.
 * */
#define ADD_TRACK_ERROR(level, ...) \
    { \
        fnc_log(level, __VA_ARGS__); \
        MObject_unref(MOBJECT(t->info)); \
        MObject_unref(MOBJECT(t->properties)); \
        free_track(t, r); \
        return NULL; \
    }

Track *add_track(Resource *r, TrackInfo *info, MediaProperties *prop_hints)
{
    Track *t;

    // TODO: search first of all in exclusive tracks

    if(r->num_tracks>=MAX_TRACKS)
        return NULL;

    if( !(t=(Track *)calloc(1, sizeof(Track))) ) 
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Memory allocation problems\n");
    
#if 0 
// we use MObject_new: that will alloc memory and exits the program if something goes wrong
    if( !(t->info = calloc(1, sizeof(TrackInfo))) )
        ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems\n");
#endif 
// we use MObject_new: that will alloc memory and exits the program if something goes wrong
    if (info)
        t->info = MObject_dup(info, sizeof(TrackInfo));
    else
        t->info = MObject_new0(TrackInfo, 1);

    MObject_destructor(t->info, trackinfo_free);


#if 0 // we use MObject_new: that will alloc memory and exits the program if something goes wrong
    if( !(t->properties = malloc(sizeof(MediaProperties))) )
        ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems\n");
#endif // we use MObject_new: that will alloc memory and exits the program if something goes wrong
    if (prop_hints)
        t->properties = MObject_dup(prop_hints, sizeof(MediaProperties));
    else
        t->properties = MObject_new0(MediaProperties, 1);

/* parser allocation no more needed.
 * we should now find the right media parser and link t->parse to the
 * correspondig index
    if( !(t->parser=add_media_parser()) )
        ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");
 */

    if( !(t->buffer = OMSbuff_new(OMSBUFFER_DEFAULT_DIM)) )
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Memory allocation problems\n");

    if ( t->info->mrl && !(t->i_stream = istream_open(t->info->mrl)) )
        ADD_TRACK_ERROR(FNC_LOG_ERR, "Could not open %s\n", t->info->mrl);

    if ( !(t->parser = mparser_find(t->properties->encoding_name)) )
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not find a valid parser\n");
    if (t->parser->init(t->properties, &t->parser_private))
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not initialize parser for %s\n",
                                        t->properties->encoding_name);
#if 0
    // shawill: just for parser trying:
    {
        uint8 tmp_dst[512];
        double timest;

        fnc_log(FNC_LOG_DEBUG, "[MT] demuxer sd init done\n");

        t->parser->get_frame(tmp_dst, sizeof(tmp_dst), &timest, t->i_stream, t->properties, t->parser_private);
    }
#endif
    t->parent = r;
    r->tracks = g_list_append(r->tracks, t);
    r->num_tracks++;

    return t;
}
#undef ADD_TRACK_ERROR

void free_track(Track *t, Resource *r)
{
    if (!t)
        return;

    MObject_unref(MOBJECT(t->info));
    mparser_unreg(t->parser, t->private_data);
    OMSbuff_free(t->buffer);
#if 0 // private data is not under Track jurisdiction!
    free(t->private_data);
#endif
    // t->calculate_timestamp=NULL; /*TODO*/
    if ( t->i_stream && IS_ISEXCLUSIVE(t->i_stream) )
        ex_track_remove(t);

    istream_close(t->i_stream);

    r->tracks = g_list_remove(r->tracks, t);
    free(t);
    r->num_tracks--;
}

ResourceDescr *r_descr_get(char *root, char *n)
{
    GList *cache_el;
    char mrl[255];

    snprintf(mrl, sizeof(mrl) - 1, "%s%s%s", root,
         (root[strlen(root) - 1] == '/') ? "" : "/", n);


    if ( !(cache_el=r_descr_find(mrl)) ) {
        Resource *r;
        if ( !(r = r_open(root, n)) ) // shawill TODO: implement pre_open cache
            return NULL;
        cache_el = r_descr_find(mrl);
        r_close(r);
    }

    return RESOURCE_DESCR(cache_el);
}

// --- static functions --- //

static inline void ex_tracks_save(GList *tracks)
{
    g_list_foreach(tracks, (GFunc)ex_track_save, NULL);
}

static void ex_track_save(Track *track, gpointer *user_data)
{
    if ( track->i_stream                 &&
         IS_ISEXCLUSIVE(track->i_stream) &&
         !g_list_find(ex_tracks, track)     )
        ex_tracks = g_list_prepend(ex_tracks, track);
}

static inline void ex_track_remove(Track *track)
{
    ex_tracks = g_list_remove(ex_tracks, track);
}

// private funcions for specific demuxer

/*! This function finds the correct demuxer for the given
 * <tt>resource_name</tt>.
 * First of all, it tries to match the <tt>resource_name</tt>'s extension with
 * one of those served by the demuxers and, if found, probes that demuxer.  If
 * no demuxer can be found this way, then it tries everyone from demuxer list.
 * \param resource_name the name of the resource.
 * \return the index of the valid demuxer in the list or -1 if it could not be
 * found.
 * */
// static int find_demuxer(resource_name n)
static int find_demuxer(InputStream *i_stream)
{
    // this int will contain the index of the demuxer already probed second
    // the extension suggestion, in order to not probe twice the same
    // demuxer that was proved to be not usable.
    int probed = -1;
    char found = 0; // flag for demuxer found.
    int i; // index
    char exts[128], *res_ext, *tkn; /* temp string containing extension
                                     * served by probing demuxer.
                                     */

    // First of all try that with matching extension: we use extension as a
    // hint of resource type.
    if ( (res_ext=strrchr(i_stream->name, '.')) && (res_ext++) ) {
        // extension present
        for (i=0; demuxers[i]; i++) {
            strncpy(exts, demuxers[i]->info->extensions, sizeof(exts));

            for (tkn=strtok(exts, ","); tkn; tkn=strtok(NULL, ",")) {
                if (!strcmp(tkn, res_ext)) {
                    fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\" "
                                "matches \"%s\" demuxer\n", res_ext,
                                demuxers[i]->info->name);
                    if (demuxers[i]->probe(i_stream) == RESOURCE_OK) {
                        fnc_log(FNC_LOG_DEBUG,
                                "[MT] probing demuxer: \"%s\" demuxer found\n",
                                demuxers[i]->info->name);
                        found = 1;
                        break;
                    }
                }
            }
            if (found) break;
        }
    }
    if (!found) {
        for (i=0; demuxers[i]; i++) {
            if ((i!=probed) && (demuxers[i]->probe(i_stream) == RESOURCE_OK))
            {
                fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\""
                                       "demuxer found\n",
                                        demuxers[i]->info->name);
                found = 1;
                break;
            }
        }
    }

    return found ? i: -1;
}

// --- Description Cache management --- //
static gint cache_cmp(gconstpointer a, gconstpointer b)
{
    return strcmp( ((ResourceDescr *)a)->info->mrl, (const char *)b );
}

static void r_descr_cache_update(Resource *r)
{
    GList *cache_el;
    ResourceDescr *r_descr=NULL;

    if ( ( cache_el = r_descr_find(r->info->mrl) ) ) {
        r_descr = RESOURCE_DESCR(cache_el);
        // TODO free ResourceDescr
        descr_cache = g_list_remove_link(descr_cache, cache_el);
        if (r_changed(r_descr)) {
            r_descr_free(r_descr);
            r_descr=NULL;
        }
    }
    if (!r_descr)
        r_descr = r_descr_new(r);

    descr_cache=g_list_prepend(descr_cache, r_descr);

    if (g_list_length(descr_cache)>MAX_DESCR_CACHE_SIZE) {
        cache_el = g_list_last(descr_cache);
        r_descr_free(RESOURCE_DESCR(cache_el));
        descr_cache = g_list_delete_link(descr_cache, cache_el);
    }
}

static ResourceDescr *r_descr_new(Resource *r)
{
    ResourceDescr *new_descr;
    MediaDescr *new_mdescr;
    GList *tracks;

    new_descr=g_new(ResourceDescr, 1);
    new_descr->media = NULL;

    new_descr->info=r->info;
    MObject_ref(r->info);
    new_descr->last_change=mrl_mtime(r->info->mrl);

    for (tracks=g_list_first(r->tracks); tracks; tracks=g_list_next(tracks)) {
        new_mdescr = g_new(MediaDescr, 1);
        new_mdescr->info = TRACK(tracks)->info;
        MObject_ref(TRACK(tracks)->info);
        new_mdescr->properties = TRACK(tracks)->properties;
        MObject_ref(TRACK(tracks)->properties);
        new_mdescr->last_change = mrl_mtime(TRACK(tracks)->info->mrl);
        new_descr->media = g_list_prepend(new_descr->media, new_mdescr);
    }
    // put the Media description in the same order of tracks.
    new_descr->media=g_list_reverse(new_descr->media);

    return new_descr;
}

static void r_descr_free(ResourceDescr *descr)
{
    GList *m_descr;

    if (!descr)
        return;

    for (m_descr=g_list_first(descr->media);
         m_descr; 
         m_descr=g_list_next(m_descr) )
    {
        MObject_unref( MOBJECT(MEDIA_DESCR(m_descr)->info) );
        MObject_unref( MOBJECT(MEDIA_DESCR(m_descr)->properties) );
    }
    g_list_free(descr->media);

    MObject_unref( MOBJECT(descr->info) );
    g_free(descr);
}

static void resinfo_free(void *resinfo)
{
    if (!resinfo)
        return;

    g_free(((ResourceInfo *)resinfo)->mrl);

    g_free(resinfo);
}

static void trackinfo_free(void *trackinfo)
{
    if (!trackinfo)
        return;

    g_free(((TrackInfo *)trackinfo)->mrl);

    g_free(trackinfo);
}

