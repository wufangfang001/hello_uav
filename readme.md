hello uav是基于RTSA的无人机场景demo  

1、互通场景，无人机端只发送视频，APP端仅接收视频  

2、编译方法：  
2.1、编译NVIDIA硬件H.264编码库jetson-ffmpeg，头文件放置于inc/nvmpi.h，库文件放置于lib/aarch64/libnvmpi.so.*  
2.2、编译demo ./build-aarch64.sh  

3、运行demo  
./hello-uav --appid 'your appid' --channel 'your channle' --token 'your token'  