hello uav是基于RTSA和native的无人机场景demo  

1、互通场景，无人机端只发送视频，APP端仅接收视频  

2、编译方法：  
2.1、编译NVIDIA硬件H.264编码库jetson-ffmpeg，头文件放置于inc/nvmpi.h，库文件放置于lib/aarch64/libnvmpi.so.*  
2.2、编译demo ./build-aarch64.sh  

3、运行demo  
./hello-uav --appid 'your appid' --channel 'your channle' --token 'your token'  

使用RTSA通道运行命令：  
LD_LIBRARY_PATH="lib:$LD_LIBRARY_PATH" ./hello-uav --appId aab8b8f5a8cd4469a63042fcfafe7063 --channelId hello-uav --fps 25 --enableRtsaSdk 1 --bitrate 1500000  

使用native通道运行命令：  
LD_LIBRARY_PATH="lib:$LD_LIBRARY_PATH" ./hello-uav --token aab8b8f5a8cd4469a63042fcfafe7063 --channelId hello-uav --fps 25 --enableRtsaSdk 0 --bitrate 1500000    --enableMultipath 1  

4、支持RTSA和native通道切换  
--enableRtsaSdk 1 切换到RTSA；--enableRtsaSdk 0 切换到native  

5、注意  
当前选择的摄像头不支持采集帧率配置，--fps务必设置为25，或者不配置默认即25帧  