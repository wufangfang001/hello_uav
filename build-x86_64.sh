gcc -O2 -fsanitize=address -fno-omit-frame-pointer -g -o hello-uav \
    src/hello_uav.c \
    src/video_capture.c \
    -Iinc -Llib/x86_64 -Wl,-rpath,lib/x86_64 -lagora-rtc-sdk -lpthread -lasan