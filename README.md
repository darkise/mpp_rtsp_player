开发板
firefly RK3399
系统
Ubuntu 16.04
需求
已经安装
rockchip_mpp

功能
获取RTSP流（H264）格式，使用mpp进行硬解码，再使用OpenGL ES 2.0进行显示
Usage:
```
cd mpp_rtsp_play
mkdir build && cd build
cmake ..
make
```
###For RK3288
1.slove the error:
```
send request. SETUP trackID=10 RTSP/1.0
CSeq: 3
User-Agent: Darkise rtsp player 1.0
Transport: RTP/AVP/TCP;unicast;interleaved=0-1


Response[RTSP/1.0 461 Unsupported transport
CSeq: 3
Date: Sat, May 29 1971 08:27:34 GMT
User-Agent: NVT

].
Response error.
setup time out
RTSP initialise error.
```
refer:https://www.cnblogs.com/lidabo/p/6553212.html
the correct request should like this:
```
send request. SETUP rtsp://192.168.1.88/Onvif/live/1/1/trackID=0 RTSP/1.0
CSeq: 3
User-Agent: Darkise rtsp player 1.0
Transport: RTP/AVP/TCP;unicast;interleaved=0-1
```
test succesfully like this:
```
linaro@tinkerboard:~/myworkspace/github/mpp_rtsp_player/build$ ./rkmpp_player 
Set non-blocking socket.
send request. OPTIONS rtsp://192.168.1.88:554/Onvif/live/1/1 RTSP/1.0
CSeq: 1
User-Agent: Darkise rtsp player 1.0


Response[RTSP/1.0 200 OK
CSeq: 1
Date: Sun, May 30 1971 02:39:29 GMT
Public: DESCRIBE, PAUSE, PLAY, SETUP, TEARDOWN, OPTIONS, SET_PARAMETER

].
send request. DESCRIBE rtsp://192.168.1.88:554/Onvif/live/1/1 RTSP/1.0
CSeq: 2
User-Agent: Darkise rtsp player 1.0
Accept: application/sdp


Response[RTSP/1.0 200 OK
CSeq: 2
User-Agent: NVT
Content-Type: application/sdp
Content-Length: 651
Content-Base: rtsp://192.168.1.88/Onvif/live/1/1/

v=0
o=admin 31626 31627 IN IP4 192.168.1.88
s=NVT
i=From NVT
c=IN IP4 192.168.1.88
t=0 0
a=range:npt=now-
a=control:rtsp://192.168.1.88/Onvif/live/1/1/
m=video 0 RTP/AVP 96
b=AS:6000
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1;profile-level-id=640032;sprop-parameter-sets=Z2QAMq2EAQwgCGEAQwgCGEAQwgCEK1AQAGDTcBAQECA=,aO48sA==
a=framesize:96 2048-1536
a=framerate:25.0
a=control:trackID=0
a=recvonly
m=audio 0 RTP/AVP 8
a=rtpmap:8 PCMA/8000
a=fmtp:8 octet-align=1;decode_buf=400
a=control:trackID=1
a=recvonly
m=application 0 RTP/AVP 107
b=AS:10
a=rtpmap:107 vnd.onvif.metadata/90000
a=control:trackID=10
a=recvonly
].
send request. SETUP rtsp://192.168.1.88/Onvif/live/1/1/trackID=0 RTSP/1.0
CSeq: 3
User-Agent: Darkise rtsp player 1.0
Transport: RTP/AVP/TCP;unicast;interleaved=0-1


Response[RTSP/1.0 200 OK
CSeq: 3
Date: Sun, May 30 1971 02:39:29 GMT
User-Agent: NVT
Session: 747012435;timeout=60
Transport: RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=f4cde45b;mode=play

].
setup response:RTSP/1.0 200 OK
CSeq: 3
Date: Sun, May 30 1971 02:39:29 GMT
User-Agent: NVT
Session: 747012435;timeout=60
Transport: RTP/AVP/TCP;unicast;interleaved=0-1;ssrc=f4cde45b;mode=play


s=NVT
i=From NVT
c=IN IP4 192.168.1.88
t=0 0
a=range:npt=now-
a=control:rtsp://192.168.1.88/Onvif/live/1/1/
m=video 0 RTP/AVP 96
b=AS:6000
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1;profile-level-id=640032;sprop-parameter-sets=Z2QAMq2EAQwgCGEAQwgCGEAQwgCEK1AQAGDTcBAQECA=,aO48sA==
a=framesize:96 2048-1536
a=framerate:25.0
a=control:trackID=0
a=recvonly
m=audio 0 RTP/AVP 8
a=rtpmap:8 PCMA/8000
a=fmtp:8 octet-align=1;decode_buf=400
a=control:trackID=1
a=recvonly
m=application 0 RTP/AVP 107
b=AS:10
a=rtpmap:107 vnd.onvif.metadata/90000
a=control:trackID=10
a=recvonly
@
send request. PLAY rtsp://192.168.1.88:554/Onvif/live/1/1 RTSP/1.0
CSeq: 4
User-Agent: Darkise rtsp player 1.0
Session: 747012435
Range: npt=0-


Response[RTSP/1.0 200 OK
CSeq: 4
Date: Sun, May 30 1971 02:39:29 GMT
User-Agent: NVT
RTP-Info: url=rtsp://192.168.1.88/Onvif/live/1/1/trackID=0
Range: npt=now-
Session: 747012435

].
mpi: mpp version: 2dc830f0 author: sliver.chen [vp8e]: add vpu1 and vpu2 vp8e supprt.
mpp_rt: NOT found ion allocator
mpp_rt: found drm allocator
h264d_dpb: dpb_size error.
decode_get_frame get info changed found
decoder require buffer w:h [2048:1536] stride [2048:1536]
picture of count: 0
picture of count: 2
picture of count: 4
picture of count: 6
picture of count: 8

```
2. remove the function of rkdrm

