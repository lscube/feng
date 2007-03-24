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
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if ENABLE_DUMPBUFF
#include <stdio.h>
#include <time.h>

#include <fenice/bufferpool.h>
#include <fenice/fnc_log.h>

#define LOG_FORMAT "%d/%b/%Y:%H:%M:%S %z"
#define MAX_LEN_DATE 30

/* \brief Dump of OMSbufferpool.
 * This function print the current status of bufferpool. This is useful only for debugging, so if ENABLE_DEBUG
 * is not defined it will be #defined to nothing.
 * The function takes two parameters that are mutual exclusive each other: If you are a consumer you should use
 * the <tt>cons</tt> parameter, while if you're the writer you should use <tt>buffer</tt> parameter.
 * If both are not NULL the first will be used.
 * */
void OMSbuff_dump(OMSConsumer * cons, OMSBuffer * buffer)
{
    static FILE *dump_file = NULL;
    uint32 i;
#define WR 0            // write_pos
#define RD 1            // read_pos
#define LR 2            // last_read_pos
    char pointers[4] = "   ";

    buffer = cons ? cons->buffer : buffer;

    if (!buffer)
        return;

    if (!dump_file) {
        char date[MAX_LEN_DATE];
        time_t now;
        const struct tm *tm;

        if (!(dump_file = fopen("OMSbuff.dump", "a")))
            return;

        time(&now);
        tm = localtime(&now);
        strftime(date, MAX_LEN_DATE, LOG_FORMAT, tm);

        fprintf(dump_file, "[%s] [new dump]\n ", date);
    }

    fprintf(dump_file, "********* BUFFERPOOL DUMP *********\n");
    fprintf(dump_file, "Buffer\n");
    fprintf(dump_file, " refs:            %u\n", buffer->control->refs);
    fprintf(dump_file, " number of slots: %u\n", buffer->control->nslots);
    fprintf(dump_file, " known slots:     %u\n", buffer->known_slots);
    if (cons) {
        fprintf(dump_file, "Consumer\n");
        fprintf(dump_file, " last_seq:            %llu\n",
            cons->last_seq);
        fprintf(dump_file, " first_rtptime:             %3.2f\n",
            cons->first_rtptime);
        fprintf(dump_file, " frames:              %d\n", cons->frames);
    }
    for (i = 0; i < buffer->known_slots; i++) {
        pointers[WR] =
            (buffer->control->write_pos == (OMSSlotPtr) i) ? 'W' : ' ';
        pointers[RD] = (cons
                && (cons->read_pos ==
                    (OMSSlotPtr) i)) ? 'R' : ' ';
        pointers[LR] = (cons
                && (cons->last_read_pos ==
                    (OMSSlotPtr) i)) ? 'L' : ' ';
        fprintf(dump_file,
            "refs: %u\tseq: %llu   \ttime: %3.2f\tslot: %u   \tnext: %d  \t%s\n",
            buffer->slots[i].refs, buffer->slots[i].slot_seq,
            buffer->slots[i].timestamp, i, buffer->slots[i].next,
            pointers);
    }
    fprintf(dump_file, "--------- BUFFERPOOL DUMP ---------\n");
}

#endif
