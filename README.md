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
### For RK3288
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
refer:https://github.com/mayunxi/mpp_rtsp_player/wiki/the-correct-rtsp

2. remove the function of rkdrm

