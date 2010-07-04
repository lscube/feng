/* -*- c -*- */

#include <stdbool.h>

#include "rtsp.h"
#include "rtp.h"

%% machine ragel_transport_header;

GSList *ragel_parse_transport_header(const char *header)
{
    GSList *transports = NULL;
    struct ParsedTransport *transport = NULL;
    int cs;
    const char *p = header, *pe = p + strlen(p) +1, *eof = pe;
    uint32_t portval = 0; uint16_t chanval = 0;

    %%{
        action start_port {
            portval = 0;
        }

        action count_port {
            portval = (portval*10) + (fc - '0');
        }

        action check_port {
            if ( portval > G_MAXUINT16 )
                portval = G_MAXUINT16;
        }

        action start_channel {
            chanval = 0;
        }

        action count_channel {
            chanval = (chanval*10) + (fc - '0');
        }

        action check_channel {
            if ( chanval > G_MAXUINT8 )
                chanval = G_MAXUINT8;
        }

        Port = digit{,5} >start_port @count_port %check_port;
        Channel = digit{,3} >start_channel @count_channel %check_channel;

        Unicast = ";unicast" %{transport->mode = TransportUnicast;};
        Multicast = ";multicast" %{transport->mode = TransportMulticast;};

        TransportParamName = ( alnum | '_' )+;
        TransportParamValue = (print - ('=' | ';' ))+;

        TransportParam = ";" . TransportParamName . ( '=' . TransportParamValue )?;

        ClientPort = ";client_port=" .
            Port%{transport->rtp_channel = portval;} .
            ( "-" . Port%{transport->rtcp_channel = portval;} );

        UnicastUDPParams = Unicast . ( ClientPort | TransportParam )+;
        MulticastUDPParams = Multicast . TransportParam+;

        UDPParams = ( UnicastUDPParams | MulticastUDPParams );

        Interleaved = ";interleaved=" .
            Channel%{transport->rtp_channel = chanval;} .
            ( "-" . Channel%{transport->rtcp_channel = chanval;} );

        TCPParams = ( Interleaved | TransportParam)+;

        Streams = ";streams=" .
            Channel%{transport->rtp_channel = chanval;} .
            ( "-" . Channel%{transport->rtcp_channel = chanval;} );

        SCTPParams = ( Streams | TransportParam)+;

        TransportUDP = ("/UDP")? %{transport->protocol = RTP_UDP; }
            . UDPParams;
        TransportTCP = "/TCP" %{transport->protocol = RTP_TCP; }
            . TCPParams;
        TransportSCTP = "/SCTP" %{transport->protocol = RTP_SCTP; }
            . SCTPParams;

        action start_transport {
            if ( transport != NULL )
                transports = g_slist_append(transports, transport);

            transport = g_slice_new0(struct ParsedTransport);
            transport->rtp_channel = transport->rtcp_channel = -1;
        }

        action error_transport {
            const char *next_transport = strchr(p, ',');

            g_slice_free(struct ParsedTransport, transport);
            transport = NULL;

            p = (next_transport == NULL) ? pe : (next_transport + 1);

            fhold; fgoto main;
        }

        TransportSpec = ( "RTP/AVP" . ( TransportUDP|TransportTCP|TransportSCTP ) )
            >(start_transport)
            <err(error_transport);

        main := TransportSpec . ( ',' . TransportSpec )* . 0;

        write data nofinal noerror;
        write init;
        write exec;
    }%%

    if ( transport != NULL )
        transports = g_slist_append(transports, transport);

    return transports;
}
