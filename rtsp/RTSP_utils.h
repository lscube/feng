
#include <fenice/rtsp.h>

#define MAX_REPLY_MESSAGE_LEN 256

typedef struct
{
    char object[255];
    char server[255];
    unsigned short port;

    description_format descr_format;
    char descr[MAX_DESCR_LENGTH];
} ConnectionInfo;

typedef struct
{
    int reply_code;
    char reply_str[MAX_REPLY_MESSAGE_LEN];
} ReplyMessage;

typedef struct
{
    ReplyMessage message;
    int got_error;
} RTSP_Error;

void set_RTSP_Error(RTSP_Error * err, int reply_code, char * message);

RTSP_Error check_forbidden_path(ConnectionInfo * cinfo);
RTSP_Error validate_url(char *url, ConnectionInfo * cinfo);
RTSP_Error check_require_header(RTSP_buffer * rtsp);
RTSP_Error extract_url(RTSP_buffer * rtsp, char * url_buffer);
RTSP_Error get_description_format(RTSP_buffer * rtsp, ConnectionInfo * cinfo);
RTSP_Error get_cseq(RTSP_buffer * rtsp);
RTSP_Error get_session_description(ConnectionInfo * cinfo);
RTSP_Error get_session_id(RTSP_buffer * rtsp, long int * session_id);

void log_user_agent(RTSP_buffer * rtsp);

extern RTSP_Error const RTSP_Ok;
extern RTSP_Error const RTSP_BadRequest;
extern RTSP_Error const RTSP_InternalServerError;
extern RTSP_Error const RTSP_Forbidden;
extern RTSP_Error const RTSP_OptionNotSupported;
extern RTSP_Error const RTSP_NotFound;
extern RTSP_Error const RTSP_SessionNotFound;
extern RTSP_Error const RTSP_Fatal_ErrAlloc;
