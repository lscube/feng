#ifndef CPD_H
#define CPD_H

#define MAX_CHARS 1024
#define OK 0
#define ERROR -1
#define WARNING 1

#include <fenice/rtp.h>
#include <fenice/mediathread.h>

typedef struct {
    double Timestamp;
    char *Content;
    size_t Size;
} MDPacket;

typedef struct {
    Sock* Socket;
    char* Filename;
    GList* Packets;
} Metadata;

int cpd_init(Resource *resource, double time);
int cpd_open(Metadata *md);
void cpd_free_client(void *arg);
int cpd_send(RTP_session *session, double now);
void cpd_server(void *args);
int cpd_seek(Resource *resource, double seekTime);
int cpd_close(Resource *resource, double time);

#endif
