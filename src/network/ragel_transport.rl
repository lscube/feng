/* -*- c -*- */

%% machine ragel_transport_header;

static gboolean ragel_parse_transport_header(RTSP_buffer *rtsp,
                                             RTP_transport *rtp_t,
                                             char *header) {
    struct ParsedTransport transport;
    int cs;
    char *p = header, *pe = p + strlen(p) +1, *eof;
    uint32_t portval; uint16_t chanval;

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

        Unicast = ";unicast" %{transport.mode = TransportUnicast;};
        Multicast = ";multicast" %{transport.mode = TransportMulticast;};

        TransportParamName = ( alnum | '_' )+;
        TransportParamValue = (print - ('=' | ';' ))+;

        TransportParam = ";" . TransportParamName . ( '=' . TransportParamValue )?;

        ClientPort = ";client_port=" .
            Port%{transport.parameters.UDP.Unicast.port_rtp = portval;} .
            ( "-" . Port%{transport.parameters.UDP.Unicast.port_rtcp = portval;} );

        UnicastUDPParams = Unicast . ( ClientPort | TransportParam )+;
        MulticastUDPParams = Multicast . TransportParam+;

        UDPParams = ( UnicastUDPParams | MulticastUDPParams );

        Interleaved = ";interleaved=" .
            Channel%{transport.parameters.TCP.ich_rtp = chanval;} .
            ( "-" . Channel%{transport.parameters.TCP.ich_rtcp = chanval;} );

        TCPParams = ( Interleaved | TransportParam)+;

        Streams = ";streams" .
            Channel%{transport.parameters.SCTP.ch_rtp = chanval;} .
            ( "-" . Channel%{transport.parameters.SCTP.ch_rtcp = chanval;} );

        SCTPParams = TransportParam+;

        TransportUDP = ("/UDP")? %{transport.protocol = TransportUDP; }
            . UDPParams;
        TransportTCP = "/TCP" %{transport.protocol = TransportTCP; }
            . TCPParams;
        TransportSCTP = "/SCTP" %{transport.protocol = TransportSCTP; }
            . SCTPParams;

        action check_transport {
            /* If the transport is valid, our work is done */
            if ( check_parsed_transport(rtsp, rtp_t, &transport) )
                return true;
        }

        TransportSpec = "RTP/AVP" . ( TransportUDP|TransportTCP|TransportSCTP )
            > { memset(&transport, 0, sizeof(transport)); }
            % check_transport;

        main := TransportSpec . ( ',' . TransportSpec )* . 0;

        write data;
        write init;
        write exec;
    }%%

    return false;
}
