cmake_minimum_required(VERSION 2.8)

project(rkmpp_player)

SET(CMAKE_C_FLAGS "-pipe -O2 -Wall -W -fPIC -DDEBUG=1")
set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include/libdrm /usr/include/rockchip ./rkdrm)

set(SOURCE_FILES main.c
        mppdecoder.c
        rtspprotocolutil.c
        rkdrm/bo.c
        rkdrm/dev.c
        rkdrm/modeset.c
        rkdrm/rkdrm.c)

add_executable(rkmpp_player ${SOURCE_FILES})
target_link_libraries(rkmpp_player rockchip_mpp pthread drm)
