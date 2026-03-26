# 需求文档

## 简介

基于现有 nvmpi 硬件编码接口和 H264 编码器实现，新增 H265 (HEVC) 编码器支持。用户可通过命令行参数在 H264 和 H265 编码器之间切换，默认使用 H264 编码器。同时需要适配 RTSA SDK 和 Native SDK 的视频发送接口，使其能够根据编码器类型正确设置视频数据类型。

## 术语表

- **H265_Encoder**: 基于 nvmpi 接口实现的 H265/HEVC 硬件视频编码器模块
- **H264_Encoder**: 现有的基于 nvmpi 接口实现的 H264 硬件视频编码器模块
- **Video_Codec_Type**: 视频编码类型枚举，用于标识当前使用的编码器类型（H264 或 H265）
- **Config**: 应用配置结构体 agora_config_t，包含视频参数和编码器类型配置
- **CLI_Parser**: 命令行参数解析模块，基于 getopt_long 实现
- **RTSA_Sender**: RTSA SDK 视频数据发送模块
- **Native_Sender**: Native SDK 视频数据发送模块
- **Main_App**: 主程序 hello_uav，负责工作线程、视频采集和编码发送流程的协调
- **Build_System**: CMake 构建系统

## 需求

### 需求 1：H265 编码器模块实现

**用户故事：** 作为开发者，我希望拥有一个与 H264 编码器接口一致的 H265 编码器模块，以便在不修改上层调用逻辑的前提下使用 H265 编码。

#### 验收标准

1. THE H265_Encoder SHALL 提供与 H264_Encoder 相同的接口集合：init、encoding、fini、generate_key_frame、set_target_bps
2. WHEN h265_encode_init 被调用时，THE H265_Encoder SHALL 使用 NV_VIDEO_CodingHEVC 编码类型创建 nvmpi 编码器实例
3. WHEN h265_encode_init 被调用时，THE H265_Encoder SHALL 接受 width、height、fps、bps 四个参数并正确配置编码器
4. WHEN h265_encode_encoding 被调用并传入 YUV420 帧数据时，THE H265_Encoder SHALL 返回编码后的 H265 数据及其长度
5. WHEN h265_encode_encoding 被调用时，THE H265_Encoder SHALL 通过 is_key_frame 输出参数正确标识关键帧
6. WHEN h265_encode_generate_key_frame 被调用时，THE H265_Encoder SHALL 强制生成一个关键帧
7. WHEN h265_encode_set_target_bps 被调用时，THE H265_Encoder SHALL 动态调整编码器目标码率
8. WHEN h265_encode_fini 被调用时，THE H265_Encoder SHALL 释放所有编码器资源和缓冲区内存
9. IF h265_encode_init 创建编码器失败，THEN THE H265_Encoder SHALL 返回 -1 错误码

### 需求 2：视频编码类型配置

**用户故事：** 作为开发者，我希望在配置结构体中增加编码器类型字段，以便系统各模块能够获取当前使用的编码器类型。

#### 验收标准

1. THE Config SHALL 包含一个 video_codec_type 字段，用于标识当前使用的视频编码类型
2. THE Video_Codec_Type SHALL 定义 H264 和 H265 两种编码类型枚举值
3. THE Config SHALL 将 video_codec_type 的默认值设置为 H264

### 需求 3：命令行参数支持编码器类型切换

**用户故事：** 作为用户，我希望通过命令行参数指定使用 H264 或 H265 编码器，以便灵活选择编码方式。

#### 验收标准

1. THE CLI_Parser SHALL 支持 `--codec` 长选项和 `-C` 短选项用于指定编码器类型
2. WHEN 用户传入 `-C h265` 或 `--codec h265` 时，THE CLI_Parser SHALL 将 video_codec_type 设置为 H265
3. WHEN 用户传入 `-C h264` 或 `--codec h264` 时，THE CLI_Parser SHALL 将 video_codec_type 设置为 H264
4. WHEN 用户未指定 `--codec` 参数时，THE CLI_Parser SHALL 保持默认值 H264
5. THE CLI_Parser SHALL 在帮助信息中显示 `--codec` 参数的用法说明

### 需求 4：主程序编码器动态选择

**用户故事：** 作为开发者，我希望主程序根据配置自动选择对应的编码器进行初始化和编码，以便实现 H264/H265 的无缝切换。

#### 验收标准

1. WHEN video_codec_type 为 H264 时，THE Main_App SHALL 调用 H264_Encoder 的接口进行编码
2. WHEN video_codec_type 为 H265 时，THE Main_App SHALL 调用 H265_Encoder 的接口进行编码
3. WHEN SDK 请求调整码率时，THE Main_App SHALL 调用当前活跃编码器的 set_target_bps 接口
4. WHEN SDK 请求关键帧时，THE Main_App SHALL 调用当前活跃编码器的 generate_key_frame 接口
5. WHEN 工作线程退出时，THE Main_App SHALL 调用当前活跃编码器的 fini 接口释放资源

### 需求 5：RTSA SDK 发送接口适配 H265

**用户故事：** 作为开发者，我希望 RTSA SDK 发送接口能够根据编码类型正确设置视频数据类型，以便接收端能正确解码 H265 视频流。

#### 验收标准

1. THE RTSA_Sender SHALL 提供一个支持指定视频数据类型的发送接口
2. WHEN 发送 H265 编码数据时，THE RTSA_Sender SHALL 将 video_frame_info_t 的 data_type 设置为 VIDEO_DATA_TYPE_H265
3. WHEN 发送 H264 编码数据时，THE RTSA_Sender SHALL 将 video_frame_info_t 的 data_type 设置为 VIDEO_DATA_TYPE_H264

### 需求 6：Native SDK 发送接口适配 H265

**用户故事：** 作为开发者，我希望 Native SDK 发送接口能够根据编码类型正确设置视频编解码类型，以便接收端能正确解码 H265 视频流。

#### 验收标准

1. THE Native_Sender SHALL 提供一个支持指定视频编解码类型的发送接口
2. WHEN 发送 H265 编码数据时，THE Native_Sender SHALL 将 EncodedVideoFrameInfo 的 codecType 设置为 VIDEO_CODEC_H265
3. WHEN 发送 H264 编码数据时，THE Native_Sender SHALL 将 EncodedVideoFrameInfo 的 codecType 设置为 VIDEO_CODEC_H264

### 需求 7：构建系统更新

**用户故事：** 作为开发者，我希望构建系统能够编译新增的 H265 编码器源文件，以便项目能正常构建。

#### 验收标准

1. THE Build_System SHALL 将 H265 编码器源文件（h265_encode.c）添加到编译源文件列表中
