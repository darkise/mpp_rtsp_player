#ifndef RTSPPROTOCOLUTIL_H
#define RTSPPROTOCOLUTIL_H
#include <stdlib.h>
#include <stdint.h>

#define RTSP_DEFAULT_PORT   (554)
#define RTSP_TIMEOUT        (3000)  // milliseconds

// Init RTSP environment
int RtspProtocolUtil_init(char const* url);
// Is the RTSP client started, if not try to start it
int isStart();
int rtsp_read();
// Try to get a packet from socket
int rtsp_packet();

#endif // RTSPPROTOCOLUTIL_H
