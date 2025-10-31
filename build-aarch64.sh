g++ -O2 -o hello-uav \
    src/hello_uav.c \
    src/video_capture.c \
    src/h264_encode.c \
    -Iinc -Llib/aarch64 -Wl,-rpath,lib/aarch64 -lagora-rtc-sdk -lnvmpi -lpthread