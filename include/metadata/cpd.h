#ifndef CPD_H
#define CPD_H

#define MAX_CHARS 1024
#define OK 0
#define ERROR -1
#define WARNING 1

#include <fenice/rtp.h>
#include <fenice/mediathread.h>

G_LOCK_DEFINE_STATIC (g_hash_global);
G_LOCK_DEFINE_STATIC (g_db_global);

typedef struct {
    double Timestamp;
    char *Content;
    size_t Size;
    uint8_t Sent;
} MDPacket;

typedef struct {
    Sock* Socket;
    char* Filename;
    GList* Packets;
} Metadata;

int cpd_open(Metadata *md);
void cpd_find_request(feng *srv, Resource *res, char *filename);
void cpd_send(RTP_session *session, double now);
void cpd_free_client(void *arg);
void* cpd_server(void *args);
void cpd_close(RTP_session *session);

#endif
