
#include <fenice/rtsp.h>

typedef struct
{
    char object[255];
    char server[255];
    unsigned short port;

    description_format descr_format;
    char descr[MAX_DESCR_LENGTH];
} ConnectionInfo;

int check_forbidden_path(ConnectionInfo * cinfo);
int validate_url(char *url, ConnectionInfo * cinfo);
int check_require_header(RTSP_buffer * rtsp);
int extract_url(RTSP_buffer * rtsp, char * url_buffer);
int get_description_format(RTSP_buffer * rtsp, ConnectionInfo * cinfo);
int get_cseq(RTSP_buffer * rtsp);
int get_session_description(ConnectionInfo * cinfo);
void log_user_agent(RTSP_buffer * rtsp);
int get_session_id(RTSP_buffer * rtsp, long int * session_id);
