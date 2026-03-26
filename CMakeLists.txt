cmake_minimum_required(VERSION 3.10)
project(hello-uav LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    src/hello_uav.c
    src/core/video_capture.c
    src/core/video_encode.c
    src/rtsa/agora_rtsa.c
    src/native/common/helper.cpp
    src/native/common/sample_common.cpp
    src/native/common/sample_connection_observer.cpp
    src/native/common/sample_event.cpp
    src/native/common/sample_local_user_observer.cpp
    src/native/sample_send_h264_pcm.cpp
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(hello-uav ${SOURCES})

target_compile_options(hello-uav PRIVATE -O2)

target_include_directories(hello-uav PRIVATE
    inc
    src/core
    src/native/agora_sdk/include
    src/native/common
)

target_link_libraries(hello-uav PRIVATE
    agora-rtc-sdk
    agora_rtc_sdk
    agora-fdkaac
    yuv
    aosl
    nvmpi
    pthread
)

# 设置 RPATH，使用 --disable-new-dtags 让间接依赖也能继承搜索路径
set_target_properties(hello-uav PROPERTIES
    BUILD_RPATH "${CMAKE_SOURCE_DIR}/lib"
)
target_link_options(hello-uav PRIVATE
    "LINKER:--disable-new-dtags"
    "LINKER:-rpath,${CMAKE_SOURCE_DIR}/lib"
)
