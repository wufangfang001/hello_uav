hello uav是基于RTSA和native的无人机场景demo  

1、互通场景，无人机端只发送视频，APP端仅接收视频  

2、编译方法：

2.1 NVIDIA 平台（nvmpi）
- 编译 NVIDIA 硬件 H.264 编码库（例如 jetson-ffmpeg），并将头文件放置于 `inc/nvmpi.h`，库文件放置于 `lib/aarch64/`（例如 `libnvmpi.so`）。

2.2 Rockchip 平台（rkmpi）
- 获取 RK 平台 SDK 或 RKMPI/RKMPP 源码并交叉编译，或使用板级 BSP 提供的库。
- 将生成的库文件放置于 `lib/rkmpi/`（例如 `librkmpi.so` 或带版本号的文件）。
- 将对应的头文件复制到 `src/core/rkmpi/include/`，或在构建配置（CMake/Makefile）中添加头文件搜索路径。

2.3 编译 demo（通用）
- 使用脚本并通过位置参数指定后端：
```
./build-aarch64.sh nvmpi
./build-aarch64.sh rkmpi
```
- 脚本行为：脚本以第一个位置参数作为后端名称（未指定时默认为 `nvmpi`），会创建并构建 `build-aarch64-<backend>` 构建目录，并通过 `-DMEDIA_BACKEND=<backend>` 传递给 CMake。
- 在编译前，请确保所选后端的库与头文件已按上文放置，或在项目 CMake 配置中指定正确的路径。

3、运行demo  
./hello-uav --appid 'your appid' --channel 'your channle' --token 'your token'  

使用RTSA通道运行命令：  
LD_LIBRARY_PATH="lib:$LD_LIBRARY_PATH" ./hello-uav --appId aab8b8f5a8cd4469a63042fcfafe7063 --channelId hello-uav --fps 25 --enableRtsaSdk 1 --bitrate 1500000  

使用native通道运行命令：  
LD_LIBRARY_PATH="lib:$LD_LIBRARY_PATH" ./hello-uav --token aab8b8f5a8cd4469a63042fcfafe7063 --channelId hello-uav --fps 25 --enableRtsaSdk 0 --bitrate 1500000    --enableMultipath 1  

可选视频尺寸参数：
- 使用短选项 `-W <width>` 或长选项 `--videoWidth <width>` 指定视频宽度（例如 `-W 1280`）。
- 使用短选项 `-H <height>` 或长选项 `--videoHeight <height>` 指定视频高度（例如 `-H 720`）。

示例（指定分辨率）：
LD_LIBRARY_PATH="lib:$LD_LIBRARY_PATH" ./hello-uav --appId aab8b8f5a8cd4469a63042fcfafe7063 --channelId hello-uav --fps 25 --bitrate 1500000 -W 1920 -H 1080

RK 平台中 `--captureFps` 表示摄像头/ISP 的实际输入帧率（默认 30），`--fps` 表示编码输出帧率。例如摄像头输出 30fps、编码需要 20fps 时使用 `--captureFps 30 --fps 20`。VI 会在送入 VENC 前完成降帧，确保编码帧率和码控使用的帧率一致。

4、支持RTSA和native通道切换  
--enableRtsaSdk 1 切换到RTSA；--enableRtsaSdk 0 切换到native  

5、注意  
当前选择的摄像头不支持修改传感器采集帧率；请通过 `--captureFps` 指定实际采集帧率，通过 `--fps` 指定编码输出帧率。
