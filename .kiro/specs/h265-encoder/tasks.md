# 实现计划：H265 编码器支持

## 概述

基于设计文档的 8 个组件，按依赖顺序逐步实现 H265 编码器功能。从底层配置和编码器模块开始，逐步向上适配 SDK 发送层、命令行解析和主程序集成，最后更新构建系统完成整体串联。

## 任务

- [x] 1. 配置结构体扩展与编码类型枚举定义
  - [x] 1.1 在 `inc/agora_config.h` 中新增 `video_codec_type_e` 枚举和 `agora_config_t` 的 `video_codec_type` 字段
    - 定义 `VIDEO_CODEC_TYPE_H264 = 0` 和 `VIDEO_CODEC_TYPE_H265 = 1` 枚举值
    - 在 `agora_config_t` 结构体末尾添加 `video_codec_type_e video_codec_type` 字段
    - _需求: 2.1, 2.2, 2.3_

- [x] 2. H265 编码器模块实现
  - [x] 2.1 创建 `src/core/h265_encode.h` 头文件
    - 声明 `h265_encode_init`、`h265_encode_fini`、`h265_encode_encoding`、`h265_encode_generate_key_frame`、`h265_encode_set_target_bps` 五个函数
    - 接口签名与 `h264_encode.h` 完全对称
    - _需求: 1.1_

  - [x] 2.2 创建 `src/core/h265_encode.c` 实现文件
    - 复制 `h264_encode.c` 的全局静态变量模式和实现逻辑
    - 将 `nvmpi_create_encoder` 的编码类型参数改为 `NV_VIDEO_CodingHEVC`
    - H265 profile 设置为 1 (Main)，level 设置为 150 (Level 5.0)
    - 日志标签和函数名使用 h265 前缀
    - _需求: 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9_

- [x] 3. Checkpoint - 确认编码器模块编译无误
  - 确保所有代码编译通过，如有问题请向用户确认。

- [x] 4. RTSA SDK 发送接口适配
  - [x] 4.1 在 `inc/agora_rtsa_sdk.h` 中将 `agora_rtsa_send_h264_data` 替换为 `agora_rtsa_send_video_data` 函数声明
    - 函数签名: `int agora_rtsa_send_video_data(uint8_t *data, size_t len, video_codec_type_e codec_type)`
    - 移除原有 `agora_rtsa_send_h264_data` 声明
    - _需求: 5.1_

  - [x] 4.2 在 `src/rtsa/agora_rtsa.c` 中将 `agora_rtsa_send_h264_data` 替换为 `agora_rtsa_send_video_data` 实现
    - 根据 `codec_type` 参数设置 `video_frame_info_t.data_type`：H264 → `VIDEO_DATA_TYPE_H264`，H265 → `VIDEO_DATA_TYPE_H265`
    - 移除原有 `agora_rtsa_send_h264_data` 实现
    - _需求: 5.2, 5.3_

- [x] 5. Native SDK 发送接口适配
  - [x] 5.1 在 `inc/agora_native_sdk.h` 中将 `agora_native_send_h264_data` 替换为 `agora_native_send_video_data` 函数声明
    - 函数签名: `int agora_native_send_video_data(uint8_t *data, size_t len, bool isKeyFrame, video_codec_type_e codec_type)`
    - 移除原有 `agora_native_send_h264_data` 声明
    - _需求: 6.1_

  - [x] 5.2 在 `src/native/sample_send_h264_pcm.cpp` 中将 `agora_native_send_h264_data` 替换为 `agora_native_send_video_data` 实现
    - 根据 `codec_type` 参数设置 `EncodedVideoFrameInfo.codecType`：H264 → `VIDEO_CODEC_H264`，H265 → `VIDEO_CODEC_H265`
    - 移除原有 `agora_native_send_h264_data` 实现
    - _需求: 6.2, 6.3_

- [x] 6. 命令行参数解析与主程序集成
  - [x] 6.1 在 `src/hello_uav.c` 中扩展命令行参数解析，新增 `-C` / `--codec` 选项
    - `short_option` 新增 `'C:'`
    - `long_option` 新增 `{ "codec", 1, NULL, 'C' }`
    - 解析 "h264" 设置 `VIDEO_CODEC_TYPE_H264`，"h265" 设置 `VIDEO_CODEC_TYPE_H265`
    - 未指定时保持默认值 H264
    - 在帮助信息和配置打印中添加 codec 相关内容
    - _需求: 3.1, 3.2, 3.3, 3.4, 3.5_

  - [x] 6.2 在 `src/hello_uav.c` 中实现编码器动态选择逻辑
    - 修改 `__sdk_target_bitrate_change` 根据 `video_codec_type` 调用对应编码器的 `set_target_bps`
    - 修改 `__sdk_key_frame_request` 根据 `video_codec_type` 调用对应编码器的 `generate_key_frame`
    - _需求: 4.3, 4.4_

  - [x] 6.3 在 `src/hello_uav.c` 中修改 `__worker` 线程函数，适配 H265 编码流程
    - 根据 `video_codec_type` 调用对应编码器的 `init`、`encoding`、`fini`
    - 将 `__agora_rtc_send_h264` 重命名为 `__agora_rtc_send_video`，调用新的通用发送接口并传入 `codec_type`
    - _需求: 4.1, 4.2, 4.5_

- [x] 7. 构建系统更新
  - [x] 7.1 在 `CMakeLists.txt` 的 `SOURCES` 列表中添加 `src/core/h265_encode.c`
    - _需求: 7.1_

- [x] 8. 最终 Checkpoint - 确保完整编译通过
  - 确保所有代码编译通过，功能串联完整，如有问题请向用户确认。

## 备注

- 每个任务引用了对应的需求编号以确保可追溯性
- Checkpoint 任务用于增量验证，确保每个阶段的代码正确性
- 实现顺序遵循自底向上的依赖关系：配置 → 编码器 → SDK 适配 → 主程序集成 → 构建系统
