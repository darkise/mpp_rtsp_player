#include "rtspprotocolutil.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <netdb.h>    // gethostbyname
#include <arpa/inet.h> // htons/ntohs

typedef struct RtspCntHeader {
    uint8_t  magic;        // 0x24
    uint8_t  channel;
    uint16_t length;
    uint8_t  payload[0];   // >> RtpHeader
} RtspCntHeader_st;

typedef struct RtpCntHeader {
#if 0
    uint8_t  version:2;
    uint8_t  padding:1;
    uint8_t  externsion:1;
    uint8_t  CSrcId:4;
    uint8_t  marker:1;
    uint8_t  pt:7;          // payload type
#else
    uint8_t  exts;
    uint8_t  type;
#endif
    uint16_t seqNo;         // Sequence number
    uint32_t ts;            // Timestamp
    uint32_t SyncSrcId;     // Synchronization source (SSRC) identifier
                            // Contributing source (SSRC_n) identifier
    uint8_t payload[0];     // Frame data
} RtpCntHeader_st;

#if DEBUG
#define rtsp_err printf
#define rtsp_log printf
void rtsp_dump(uint8_t* data, int sz)
{
   int i = 0;
   for (i = 0; i < sz; i++) {
       printf("%02x ", data[i]);
       if ((i & 0x0f) == 0x0f) printf("\n");
   }
   if ((sz & 0x0f) != 0) printf("\n");
}
#else
#define rtsp_err
#define rtsp_log
#define rtsp_dump
#endif

#define RTSP_VERSIION       "RTSP/1.0"
#define RTSP_SESSION_NAME   "Session:"
#define RTSP_USERAGENT      "User-Agent: Darkise rtsp player 1.0\r\n"

// Command handler
char _url[256];
char host[64];
int port = RTSP_DEFAULT_PORT;
int rtspSocket = -1;
int rtspTimeout = RTSP_TIMEOUT;
// RTSP control sequeue number
int CSeq;
// Use only UDP protocol
int clientPort[2] = { 37477, 37478 };
int serverPort[2];
// Get from SETUP's response, PLAY and TEARDOWN need it
char sessionId[32];
/* Media attributes:
 * a=x-dimensions:1920,1080
 * a=control:rtsp://192.168.199.30:554/h264/ch1/main/av_stream/trackID=1
 * a=rtpmap:96 H264/90000
 * a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z00AKY2NQDwBE/LCAAAOEAACvyAI,aO44gA==
 * a=Media_header:MEDIAINFO=494D4B48010100000400000100000000000000000000000000000000000000000000000000000000;
 * Concerned only
 */
int video_width, video_height;
char control[256];

int rtp_size = 16*1024*1024;
uint8_t* rtp_content; // Buffer for rtp
int rtp_read, rtp_write;

/// Operations:
// RTSP operations
static int options(int to);
static int describe(int to);
static int setup(int to);
static int play(int to);
static int get_params(int to);
static int teardown(int to);
static int _setup_interleaved(int to);
static int _set_range(int to);
//
static int _send_request(char const* req, int size);
static int _wait_response(int to, char resp[], size_t size);
static int _parse_session(char const* resp);   // SETUP
static int _parse_sdp(char const* resp);       // DESCRIBE

// Rtp content operations
static int rtsp_init();

int RtspProtocolUtil_init(char const* url)
{
#define RTSP_URL_PREFIX "rtsp://"

    int pos = sizeof(RTSP_URL_PREFIX)-1;
    CSeq = 1;
    strcpy(_url, url);

    // Parse URL, get configuration
    // the url MUST start with "rtsp://"
    if (strncmp(_url, RTSP_URL_PREFIX, pos) != 0) {
        rtsp_err("URL not started with \"rtsp://\".\n");
        return -1;
    }

    // get host
    while (_url[pos] != '\0' && _url[pos] != ':' && _url[pos] != '/') {
        host[pos-7] = _url[pos];
        pos++;
    }
    host[pos-7] = '\0';
    // get port, if it didn't set in URL use default value 554
    if (_url[pos] == ':') {
        pos++;
        port = 0;
        while (_url[pos] <= '9' && _url[pos] >= '0') {
            port *= 10;
            port += _url[pos] - '0';
            pos++;
        }
    }
    if (_url[pos] != '/') {
        rtsp_err("URL format error.\n");
        return -1;
    }
    if (port <= 0) port = RTSP_DEFAULT_PORT;

    return rtsp_init();
}

static int rtsp_init()
{
    CSeq = 1;
    struct hostent *hp;
    struct sockaddr_in server;
    // Rtp content buffer
    rtp_content = (uint8_t*)malloc(rtp_size);

    // Get server IP
    hp = gethostbyname(host);
    if (NULL == hp) {
        rtsp_err("gethostbyname(%s) error.\n", host);
        return -1;
    }
    // Connect to Server
    rtspSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (rtspSocket < 0) {
        rtsp_err("Create socket failed.\n");
        return -1;
    }
    memset(&server, 0, sizeof(struct sockaddr_in));
    memcpy(&(server.sin_addr), hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)(port));
    if (connect(rtspSocket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) != 0) {
        rtsp_err("Connect to server [%x:%d] error.\n", server.sin_addr.s_addr, server.sin_port);
        close(rtspSocket);
        rtspSocket = -1;
        return -1;
    }

    /// Set socket to non-blocking
    rtsp_log("Set non-blocking socket.\n");
    int on = 1;
    int rc = ioctl(rtspSocket, FIONBIO, (char *)&on);
    if (rc < 0) {
        close(rtspSocket);
        rtspSocket = -1;
        return -1;
    }

    /** RTSP控制协议初始化 */
    // OPTIONS
    if (options(rtspTimeout)) {
        close(rtspSocket);
        rtspSocket = -1;
        return -1;
    }
    // DESCRIBE
    if (describe(rtspTimeout)) {
        close(rtspSocket);
        rtspSocket = -1;
        return -1;
    }
    // SETUP
    if (setup(rtspTimeout)) {
        close(rtspSocket);
        rtspSocket = -1;
        return -1;
    }
    // PLAY
    if (play(rtspTimeout)) {
        close(rtspSocket);
        rtspSocket = -1;
        return -1;
    }

    // Success
    return 0;
}

int isStart()
{
    // 检查环境是否准备就绪
    if (rtspSocket >= 0 && CSeq > 4) {
        return 1; 
    }
    if (rtspSocket >= 0) {
        close(rtspSocket);
        rtspSocket = -1;
    }
    rtsp_init();
    ///socket RTSP play已完成
    if (rtspSocket < 0 || CSeq <= 4)
        return 0;
    // Normal
    return 1;
}

/**
 * 尝试从socket中获取rtsp数据
 **/
int rtsp_read()
{
    static uint8_t buff[2048];
    // socket 有数据吗？
    int rcvs = recv(rtspSocket, buff, sizeof(buff), 0);
    if (rcvs < 0) {
        if (errno != EAGAIN) {
        // ERROR
        close(rtspSocket);
        rtspSocket = -1;
        }
    }
    else if (rcvs > 0) {
        //rtsp_dump(buff, rcvs);
        // 移动缓冲区剩余内容到页首
        if (rtp_read != rtp_write) {
            if (rtp_read != 0) {
                memmove(rtp_content, rtp_content+rtp_read, rtp_write - rtp_read);
                rtp_write -= rtp_read;
                rtp_read = 0;
            }
        }
        else {
            rtp_read = rtp_write = 0;
        }
        // 复制到缓冲区
        memcpy(rtp_content+rtp_write, buff, rcvs);
        rtp_write += rcvs;
    }
    //else /* if (0 == rcvs) */ {    }

    return rcvs;
}

#define _rtsp_remaining ((size_t)(rtp_read>rtp_write?(rtp_size - (rtp_read - rtp_write)):(rtp_write - rtp_read)))
extern uint8_t         *packet_buffer;
extern uint32_t        packet_wpos;
int rtsp_packet()
{
    int hasNext = 0;
    if (_rtsp_remaining <= sizeof(RtspCntHeader_st))
        return 0;


    do {
        hasNext = 0;
        RtspCntHeader_st* rtspH = (RtspCntHeader_st*)(rtp_content+rtp_read);
        if (0x24 != rtspH->magic) {
            rtsp_err("Magic number error. %02x\n", rtspH->magic);
            // Magic number ERROR, discarding all the data
            rtp_read = rtp_write = 0;
            break;
        }
        size_t rtsplen = ntohs(rtspH->length);
        if (rtsplen > _rtsp_remaining - sizeof(RtspCntHeader_st)) {
            rtsp_log("No enough data. %lu|%lu\n", rtsplen, _rtsp_remaining);
            // No enough data, try next loop
            break;
        }
        hasNext = 1;
        RtpCntHeader_st* rtpH = (RtpCntHeader_st*)rtspH->payload;
        if (0x60 != (rtpH->type & 0x7f)) {
            rtsp_err("No video stream %02x.\n", rtpH->type);
            // 不是RTP视频数据，不处理
            rtp_read += (sizeof(RtspCntHeader_st) + rtsplen);
            continue;
        }
        // 将数据复制到packet buffer中，数据足够时使用mpp解码
        uint8_t h1 = rtpH->payload[0];
        uint8_t h2 = rtpH->payload[1];
        uint8_t nal = h1 & 0x1f;
        uint8_t flag  = h2 & 0xe0;
        int paylen = rtsplen - sizeof(RtpCntHeader_st);
        if (0x1c == nal) {
            if (0x80 == flag) {
                packet_buffer[packet_wpos++] = 0;
                packet_buffer[packet_wpos++] = 0;
                packet_buffer[packet_wpos++] = 0;
                packet_buffer[packet_wpos++] = 1;
                packet_buffer[packet_wpos++] = ((h1 & 0xe0) | (h2 & 0x1f));
            }
            memcpy(packet_buffer + packet_wpos, &(rtpH->payload[2]), paylen - 2);
            packet_wpos += (paylen - 2);
        }
        else {
            packet_buffer[packet_wpos++] = 0;
            packet_buffer[packet_wpos++] = 0;
            packet_buffer[packet_wpos++] = 0;
            packet_buffer[packet_wpos++] = 1;
            memcpy(packet_buffer + packet_wpos, rtpH->payload, paylen);
            packet_wpos += paylen;
        }
        // Move read pointer
        rtp_read += (paylen + sizeof(struct RtpCntHeader) + sizeof(struct RtspCntHeader));
        // Got a packet
        return packet_wpos;

    } while (hasNext && _rtsp_remaining > sizeof(RtspCntHeader_st));

    return 0;
}

static int options(int ms)
{
    /*
     * OPTIONS url RTSP/1.0\r\n
     * CSeq: n\r\n
     * User-Agent: Darkise rtsp player\r\n
     */
#define OPTIONS_CMD "OPTIONS %s " RTSP_VERSIION "\r\nCSeq: %d\r\n" RTSP_USERAGENT "\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), OPTIONS_CMD, _url, CSeq);
    _send_request(cmd, size);
    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response

    CSeq++;
    return 0;
}

static int describe(int ms)
{
    /*
     * DESCRIBE url RTSP/1.0\r\n
     * CSeq: n\r\n
     * User-Agent: Darkise rtsp player\r\n
     * Accept: application/sdp\r\n
     */
#define DESCRIBE_CMD "DESCRIBE %s " RTSP_VERSIION "\r\n""CSeq: %d\r\n" RTSP_USERAGENT "Accept: application/sdp\r\n\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), DESCRIBE_CMD, _url, CSeq);
    // Send command the RTSP server
    _send_request(cmd, size);
    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response
    _parse_sdp(resp);

    CSeq++;

    return 0;
}

static int setup(int ms)
{
    return _setup_interleaved(ms);
}

static int play(int ms)
{
    /*
     * PLAY url RTSP/1.0\r\n
     * CSeq: 1\r\n
     * User-Agent: Darkise rtsp player\r\n
     * Session: \r\n
     * Range: npt=0-\r\n
     */
#define PLAY_CMD "PLAY %s " RTSP_VERSIION "\r\nCSeq: %d\r\n" RTSP_USERAGENT "Session: %s\r\nRange: npt=0-\r\n\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), PLAY_CMD, _url, CSeq, sessionId);
    _send_request(cmd, size);

    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response

    CSeq++;
    return 0;
}

static int get_params(int ms)
{
    /*
     * GET_PARAMETER url RTSP/1.0\r\n
     * CSeq: 1\r\n
     * User-Agent: Darkise rtsp player\r\n
     * Session: \r\n
     */

#define GET_PARAMETER_CMD "GET_PARAMETER %s " RTSP_VERSIION "\r\nCSeq: %d\r\n" RTSP_USERAGENT "Session: %s\r\n\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), GET_PARAMETER_CMD, _url, CSeq, sessionId);
    _send_request(cmd, size);

    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response

    CSeq++;
    return 0;
}

static int teardown(int ms)
{
    /*
     * TEARDOWN url RTSP/1.0\r\n
     * CSeq: 1\r\n
     * User-Agent: Darkise rtsp player\r\n
     * Session: \r\n
     */
#define TEARDOWN_CMD "TEARDOWN %s " RTSP_VERSIION "\r\nCSeq: %d\r\n" RTSP_USERAGENT "Session: %s\r\n\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), TEARDOWN_CMD, _url, CSeq, sessionId);
    _send_request(cmd, size);

    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response

    CSeq++;
    return 0;
}

static int _setup_interleaved(int ms)
{
    /*
     * SETUP (attribute control) RTSP/1.0\r\n
     * CSeq: n\r\n
     * User-Agent: Darkise rtsp player\r\n
     * Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n
     */
#define SETUPI_CMD_I "SETUP %s " RTSP_VERSIION "\r\nCSeq: %d\r\n" RTSP_USERAGENT "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), SETUPI_CMD_I, control, CSeq);
    _send_request(cmd, size);

    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response
    _parse_session(resp);

    CSeq++;

    return 0;
}
static int _set_range(int ms)
{
    /*
     * SETUP (attribute control) RTSP/1.0\r\n
     * CSeq: n\r\n
     * User-Agent: Darkise rtsp player\r\n
     * Transport: RTP/AVP;unicast;client_port=37477-37478
     */
#define SETUP_CMD_R "SETUP %s " RTSP_VERSIION "\r\nCSeq: %d\r\n" RTSP_USERAGENT "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n"
    char cmd[1024];
    int size = snprintf(cmd, sizeof(cmd), SETUP_CMD_R, control, CSeq, clientPort[0], clientPort[1]);
    _send_request(cmd, size);

    // Waiting for response
    char resp[2048];
    if (_wait_response(ms, resp, sizeof(resp))) {
        return -1;
    }
    // Parse response, want value include: session, server_port
    _parse_session(resp);
    ///Transport: RTP/AVP;unicast;client_port=36346-36347;server_port=8236-8237;ssrc=7fa9c094;mode="play"

    // Set up UDP connection twice
    /// send HEX { ce fa ed fe } to server port

    CSeq++;

    return 0;
}

static int _send_request(char const* req, int size)
{
    rtsp_log("send request. %.*s\n", size, req);
    ssize_t snd = 0;
    if (rtspSocket < 0) {
        rtsp_err("Connection to server has not been set up.\n");
        return -1;
    }
    ssize_t s;
    do {
        s = send(rtspSocket, req + snd, size - snd, 0);
        if (s <= 0) {
            rtsp_err("Send request error. %s.\n", strerror(errno));
            return -1;
        }
        snd += s;
    } while (snd < size);

    return 0;
}

static int _wait_response(int ms, char* resp, size_t size)
{
    int rc = 0;
    struct pollfd fds;
    //int poll(struct pollfd *fds, nfds_t nfds, int timeout);
    if (rtspSocket < 0) {
        rtsp_err("Connection to server has not been set up.\n");
        return -1;
    }
    memset(&fds, 0, sizeof(fds));
    fds.fd = rtspSocket;
    fds.events = POLLIN;

    rc = poll(&fds, 1, ms);
    if (rc < 0) {
        rtsp_err("poll call error. %s.\n", strerror(errno));
        return -1;
    }
    else if (0 == rc) {
        rtsp_log("Time out.\n");
    }

    // Receiving
    int rcvs = recv(rtspSocket, resp, size, 0);
    rtsp_log("Response[%.*s].\n", rcvs, resp);
    // Is response ok?
    /// checking "RTSP/1.0 200 OK \r\n"
#define RTSP_SUCESS RTSP_VERSIION" 200 OK\r\n"
    if (strncmp(resp, RTSP_SUCESS, sizeof(RTSP_SUCESS)-1) != 0) {
        rtsp_err("Response error.\n");
        return -1;
    }
    return 0;
}

static int _parse_session(char const* resp)
{
    // Get session ID
    /// Session:       1416676415;timeout=60
    char const* pr = strstr(resp, RTSP_SESSION_NAME);
    if (NULL == pr) {
        rtsp_err("Not " RTSP_SESSION_NAME " entry.\n");
        return -1;
    }
    pr += (sizeof(RTSP_SESSION_NAME) - 1);
    // Skip blank or '\t'
    while (' ' == *pr || '\t' == *pr) pr++;
    // Copy the number character
    int w = 0;
    while (*pr >= '0' && *pr <= '9') {
        sessionId[w++] = *pr++;
    }
    sessionId[w] = '\0';

    return 0;
}

static int _parse_sdp(char const* resp)
{
#define SDP_XDIM    "x-dimensions:"
#define SDP_CONTROL "control:"
    // Concerned
    // a=x-dimensions:1920,1080
    // a=control:rtsp://192.168.199.30:554/h264/ch1/main/av_stream/trackID=1
    // only
    // Get content
    char const* pr = strstr(resp, "\r\n\r\n");
    if (NULL == pr) {
        rtsp_err("Parse SDP cannot find content.\n");
        return -1;
    }
    // SDP begining
    pr += 4;
    do {
        if ('\0' == *pr) break;
        // Concerned started with "a=" only
        if (*pr != 'a' || *(pr+1) != '=') {
            // Next line
            goto next_line;
        }
        // Skip "a="
        pr += 2;
        if (0 == strncmp(pr, SDP_XDIM, sizeof(SDP_XDIM)-1)) {
            pr += (sizeof(SDP_XDIM)-1);
            // Parse video dimensions
            // Width
            // Skip blank or '\t'
            while (' ' == *pr || '\t' == *pr) pr++;
            video_width = 0;
            while (*pr >= '0' && *pr <= '9') {
                video_width *= 10;
                video_width += *pr - '0';
                pr++;
            }
            // Height
            pr++; // Skip ","
            // Skip blank or '\t'
            while (' ' == *pr || '\t' == *pr) pr++;
            video_height = 0;
            while (*pr >= '0' && *pr <= '9') {
                video_height *= 10;
                video_height += *pr - '0';
                pr++;
            }
        }
        else if (0 == strncmp(pr, SDP_CONTROL, sizeof(SDP_CONTROL)-1)) {
            pr += (sizeof(SDP_CONTROL)-1);
            // Parse control
            // Skip blank or '\t'
            while (' ' == *pr || '\t' == *pr) pr++;
            int w = 0;
            while (*pr != '\0' && *pr != '\r' && *pr != '\n')
                control[w++] = *pr++;
            control[w] = '\0';
        }

next_line:
        while (*pr != '\0' && *pr != '\r' && *pr != '\n') pr++;
        if ('\r' == *pr)
            pr += 2;
        else if ('\n' == *pr)
            pr++;
    } while (1);

    return 0;
}
