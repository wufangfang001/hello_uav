cmake_minimum_required(VERSION 3.10)
project(hello-uav LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MEDIA_BACKEND "nvmpi" CACHE STRING "Media backend implementation")
set_property(CACHE MEDIA_BACKEND PROPERTY STRINGS nvmpi rkmpi)

set(COMMON_LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

set(COMMON_SOURCES
    src/hello_uav.c
    src/core/media_pipeline.c
    src/rtsa/agora_rtsa.c
    src/native/common/helper.cpp
    src/native/common/sample_common.cpp
    src/native/common/sample_connection_observer.cpp
    src/native/common/sample_event.cpp
    src/native/common/sample_local_user_observer.cpp
    src/native/sample_send_h264_pcm.cpp
)

set(MEDIA_BACKEND_SOURCES)
set(MEDIA_BACKEND_INCLUDE_DIRS)
set(MEDIA_BACKEND_LINK_DIRS)
set(MEDIA_BACKEND_LIBS)

if(MEDIA_BACKEND STREQUAL "nvmpi")
    list(APPEND MEDIA_BACKEND_SOURCES
        src/core/nvmpi/media_backend_nvmpi.c
        src/core/nvmpi/video_capture.c
        src/core/nvmpi/video_encode.c
    )
    list(APPEND MEDIA_BACKEND_INCLUDE_DIRS
        src/core/nvmpi
    )
    list(APPEND MEDIA_BACKEND_LINK_DIRS
        ${CMAKE_SOURCE_DIR}/lib/nvmpi
    )
    list(APPEND MEDIA_BACKEND_LIBS
        nvmpi
    )
elseif(MEDIA_BACKEND STREQUAL "rkmpi")
    list(APPEND MEDIA_BACKEND_SOURCES
        src/core/rkmpi/media_backend_rkmpi.c
    )
    list(APPEND MEDIA_BACKEND_INCLUDE_DIRS
        src/core/rkmpi/include
    )
    list(APPEND MEDIA_BACKEND_LINK_DIRS
        ${CMAKE_SOURCE_DIR}/lib/rkmpi
    )
    list(APPEND MEDIA_BACKEND_LIBS
        rockit
        rockchip_mpp
    )
else()
    message(FATAL_ERROR "Unsupported MEDIA_BACKEND: ${MEDIA_BACKEND}")
endif()

set(SOURCES
    ${COMMON_SOURCES}
    ${MEDIA_BACKEND_SOURCES}
)

link_directories(${COMMON_LIB_DIR} ${MEDIA_BACKEND_LINK_DIRS})

add_executable(hello-uav ${SOURCES})

target_compile_options(hello-uav PRIVATE -O2)

target_include_directories(hello-uav PRIVATE
    inc
    src/core
    ${MEDIA_BACKEND_INCLUDE_DIRS}
    src/native/agora_sdk/include
    src/native/common
)

target_link_libraries(hello-uav PRIVATE
    agora-rtc-sdk
    agora_rtc_sdk
    agora-fdkaac
    yuv
    aosl
    ${MEDIA_BACKEND_LIBS}
    pthread
)

# 设置 RPATH，使用 --disable-new-dtags 让间接依赖也能继承搜索路径
set(HELLO_UAV_RPATH "${COMMON_LIB_DIR}")
foreach(link_dir IN LISTS MEDIA_BACKEND_LINK_DIRS)
    list(APPEND HELLO_UAV_RPATH "${link_dir}")
endforeach()

set_target_properties(hello-uav PROPERTIES
    BUILD_RPATH "${HELLO_UAV_RPATH}"
)

target_link_options(hello-uav PRIVATE
    "LINKER:--disable-new-dtags"
    "LINKER:-rpath,${COMMON_LIB_DIR}"
)

foreach(link_dir IN LISTS MEDIA_BACKEND_LINK_DIRS)
    target_link_options(hello-uav PRIVATE
        "LINKER:-rpath,${link_dir}"
    )
endforeach()
