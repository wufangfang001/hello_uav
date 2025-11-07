hello uav是基于RTSA和native的无人机场景demo  

1、互通场景，无人机端只发送视频，APP端仅接收视频  

2、编译方法：  
2.1、编译NVIDIA硬件H.264编码库jetson-ffmpeg，头文件放置于inc/nvmpi.h，库文件放置于lib/aarch64/libnvmpi.so.*  
2.2、编译demo ./build-aarch64.sh  

3、运行demo  
./hello-uav --appid 'your appid' --channel 'your channle' --token 'your token'  

4、支持RTSA和native通道切换  
--enableRtsaSdk 1 切换到RTSA；--enableRtsaSdk 0 切换到native  

5、注意  
当前选择的摄像头不支持采集帧率配置，--fps务必设置为25，或者不配置默认即25帧  