g++ -O2 -o hello-uav \
    src/hello_uav.c \
    src/core/video_capture.c \
    src/core/h264_encode.c \
    src/rtsa/agora_rtsa.c \
    src/native/common/helper.cpp \
    src/native/common/sample_common.cpp \
    src/native/common/sample_connection_observer.cpp \
    src/native/common/sample_event.cpp \
    src/native/common/sample_local_user_observer.cpp \
    src/native/sample_send_h264_pcm.cpp \
    -Iinc -Isrc/core -Isrc/native/agora_sdk/include -Isrc/native/common \
    -Llib -Wl,-rpath,lib -lagora-rtc-sdk -lagora_rtc_sdk -lagora-fdkaac -lyuv -laosl -lnvmpi -lpthread
