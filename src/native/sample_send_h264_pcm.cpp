//  Agora RTC/MEDIA SDK
//
//  Created by Jay Zhang in 2020-04.
//  Copyright (c) 2020 Agora.io. All rights reserved.
//

//video
//   ******************         --------         *************         ------------       *********        **********         ************       ------------        ***********       --------
//  {SDK::capture video}  ==>  |raw data|  ==>  {SDK::encode B}  ==>  |encoded data| ==> {SDK::send}  ==> {AGORA::VOS}  ==>  {SDK::receive} ==> |encoded data|  ==> {SDK::decode} ==> |raw data|
//   ******************         --------         *************         ------------       *********        **********         ************       ------------        ***********       --------
//                                                                                  sample send h264(this sample)                              sample receive h264                 

//This sample will show how to use the SDK to send the encoded_Video to the Agora_channel
//As a user,you should papera the encoded_Video and create the Agora_service,Agora_connection, Agora_EncodedImage_sender and Agora_local_video_track
//You should parse the encoded_Video and send to sdk one frame by one frame.(by use the AGORA_API: Sender->sendEncodedVideoImage())
//The class HelperH264FileParser is a common_helper class to parse the h264 video 
//And all Agora necessary Classes builded in main() function
//Last, the sendVideoThread call Sender->sendEncodedVideoImage() to send the encoded videoFrame

//The sdk also provide lots of call_back functions to help user get the network states , local states and  peer states
//The callback functions can be found in **observer , you should register the observer first.

// the SDKapi call flows:
// service = createAndInitAgoraService()
//     connection = service->createRtcConnection()
//         connection->registerObserver()
//             connection->connect()
//         factory = service->createMediaNodeFactory()
//             VideoSender = factory->createVideoEncodedImageSender();
//                 VideoTrack = service->createCustomVideoTrack()
//      connection->getLocalUser()->publishAudio(VideoTrack);
//                 Sender->sendEncodedVideoImage()  



//audio
//   ******************         --------         *************         ------------       *********        **********         ************       ------------        ***********       --------
//  {SDK::capture audio}  ==>  |raw data|  ==>  {SDK::encode B}  ==>  |encoded data| ==> {SDK::send}  ==> {AGORA::VOS}  ==>  {SDK::receive} ==> |encoded data|  ==> {SDK::decode} ==> |raw data|
//   ******************         --------         *************         ------------       *********        **********         ************       ------------        ***********       --------
//                        sample send pcm(this sample)                sample send opus                                                                                            sample receive pcm

//This sample will show how to use the SDK to send the raw AudioData(pcm) to the Agora_channel
//As a user,you should papera the audio and create the Agora_service,Agora_connection, Agora_audio_pcm_sender and Agora_local_audio_track
//You should parse the audio and send to sdk one frame by one frame.(by use the AGORA_API: Sender->sendAudioPcmData())
//And all Agora necessary Classes builded in main() function
//Last, the sendAudioThread call Sender->sendAudioPcmData() to send the audioFrame

//The sdk also provide lots of call_back functions to help user get the network states , local states and  peer states
//The callback functions can be found in **observer , you should register the observer first.


// the SDKapi call flows:
// service = createAndInitAgoraService()
//     connection = service->createRtcConnection()
//         connection->registerObserver()
//         connection->connect()
//         factory = service->createMediaNodeFactory()
//             AudioSender = factory->createAudioPcmDataSender();
//                 AudioTrack = service->createCustomAudioTrack();
//      connection->getLocalUser()->publishAudio(audioTrack);
//                 Sender->sendAudioPcmData()  

//The destruct order of all the class can be find in the main function end.

// Wish you have a great experience with Agora_SDK!


#include <csignal>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>

#include "IAgoraService.h"
#include "NGIAgoraRtcConnection.h"
#include "common/helper.h"
#include "common/log.h"
#include "common/sample_common.h"
#include "common/sample_connection_observer.h"
#include "common/sample_local_user_observer.h"
#include "agora_config.h"
#include "agora_native_sdk.h"

#include "NGIAgoraAudioTrack.h"
#include "NGIAgoraLocalUser.h"
#include "NGIAgoraMediaNodeFactory.h"
#include "NGIAgoraMediaNode.h"
#include "NGIAgoraVideoTrack.h"

#define DEFAULT_CONNECT_TIMEOUT_MS (3000)
static uint32_t g_video_fps = 25;

static agora::base::IAgoraService* service = nullptr;
static agora::agora_refptr<agora::rtc::IRtcConnection> connection = nullptr;
static std::shared_ptr<SampleConnectionObserver> connObserver = nullptr;
static std::shared_ptr<SampleLocalUserObserver> localUserObserver = nullptr;
static agora::agora_refptr<agora::rtc::IMediaNodeFactory> factory = nullptr;
static agora::agora_refptr<agora::rtc::IAudioPcmDataSender> audioFrameSender = nullptr;
static agora::agora_refptr<agora::rtc::ILocalAudioTrack> customAudioTrack = nullptr;
static agora::agora_refptr<agora::rtc::IVideoEncodedImageSender> videoFrameSender = nullptr;
static agora::agora_refptr<agora::rtc::ILocalVideoTrack> customVideoTrack = nullptr;
static bool g_initialized = false;

static f_sdk_target_bitrate_change g_fun_bps_change = NULL;
static f_sdk_key_frame_request     g_fun_keyframe_request = NULL;
static f_sdk_should_stop           g_fun_stop = NULL;

void agora_native_fini(void);

void agora_native_target_bitrate_change(uint32_t target_bps)
{
  if (g_fun_bps_change) {
    g_fun_bps_change(target_bps);
  }
}

void agora_native_keyframe_request(void)
{
  if (g_fun_keyframe_request) {
    g_fun_keyframe_request();
  }
}

static void __notify_function_register(agora_config_t *config)
{
  g_fun_bps_change       = config->f_bps_change;
  g_fun_keyframe_request = config->f_key_frame;
  g_fun_stop             = config->f_stop;
}

int agora_native_init(agora_config_t *config)
{
  if (g_initialized) {
    AG_LOG(WARNING, "Agora service already initialized!");
    return 0;
  }

  g_video_fps = config->video_fps;
  __notify_function_register(config);

  // Create Agora service
  service = createAndInitAgoraService(false, true, true);
  if (!service) {
    AG_LOG(ERROR, "Failed to create Agora service!");
    return -1;  // 修复：添加返回
  }

  // Create Agora connection
  agora::rtc::RtcConnectionConfiguration ccfg;
  ccfg.autoSubscribeAudio = false;
  ccfg.autoSubscribeVideo = false;
  ccfg.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;
  //ccfg.maxSendBitrate = config->video_bps;
  connection = service->createRtcConnection(ccfg);
  if (!connection) {
    AG_LOG(ERROR, "Failed to create Agora connection!");
    service->release();  // 修复：释放service
    service = nullptr;
    return -1;
  }

  setLowDelay(connection);

  if (config->b_enable_multi_path) {
    if (connection->enableMultipath(true) != 0) {
      AG_LOG(ERROR, "enable multipath failed!");
    } else {
      AG_LOG(INFO, "enable multipath success!");
    }
  }

  std::string localIP;
  getLocalIP(connection, localIP);
  if (setLocalIP(connection, localIP)){
    AG_LOG(ERROR, "set local IP to %s error!", localIP.c_str());
    connection = nullptr;
    service->release();
    service = nullptr;
    return -1;
  }

  // Register observers
  connObserver = std::make_shared<SampleConnectionObserver>();
  connection->registerObserver(connObserver.get());
  connection->registerNetworkObserver(connObserver.get());

  localUserObserver = std::make_shared<SampleLocalUserObserver>(connection->getLocalUser());

  // Connect to Agora channel
  if (connection->connect(config->appid, config->channel, NULL, "0")) {
    AG_LOG(ERROR, "Failed to connect to Agora channel!");
    // 修复：清理已创建的资源
    connection->unregisterObserver(connObserver.get());
    connection->unregisterNetworkObserver(connObserver.get());
    connection = nullptr;
    connObserver.reset();
    localUserObserver.reset();
    service->release();
    service = nullptr;
    return -1;
  }

  // Create media node factory
  factory = service->createMediaNodeFactory();
  if (!factory) {
    AG_LOG(ERROR, "Failed to create media node factory!");
    // 修复：清理资源
    agora_native_fini();
    return -1;
  }

  // Create audio pipeline
  audioFrameSender = factory->createAudioPcmDataSender();
  if (!audioFrameSender) {
    AG_LOG(ERROR, "Failed to create audio data sender!");
    agora_native_fini();
    return -1;
  }

  customAudioTrack = service->createCustomAudioTrack(audioFrameSender);
  if (!customAudioTrack) {
    AG_LOG(ERROR, "Failed to create audio track!");
    agora_native_fini();
    return -1;
  }

#if 0
  // 修复：发布音频轨道
  if (connection->getLocalUser()->publishAudio(customAudioTrack)) {
    AG_LOG(ERROR, "Failed to publish audio track!");
    agora_native_fini();
    return -1;
  }
#endif

  // Create video pipeline
  videoFrameSender = factory->createVideoEncodedImageSender();
  if (!videoFrameSender) {
    AG_LOG(ERROR, "Failed to create video frame sender!");
    agora_native_fini();
    return -1;
  }

  agora::rtc::SenderOptions option;
  option.ccMode = agora::rtc::TCcMode::CC_ENABLED;
  option.targetBitrate = config->video_bps / 1000;
  customVideoTrack = service->createCustomVideoTrack(videoFrameSender, option);
  if (!customVideoTrack) {
    AG_LOG(ERROR, "Failed to create video track!");
    agora_native_fini();
    return -1;
  }

  // Publish video track
  if (connection->getLocalUser()->publishVideo(customVideoTrack)) {
    AG_LOG(ERROR, "Failed to publish video track!");
    agora_native_fini();
    return -1;
  }

  // 修复：检查连接是否真正成功
  if (0 != connObserver->waitUntilConnected(DEFAULT_CONNECT_TIMEOUT_MS)) {
    AG_LOG(ERROR, "Failed to connect within timeout!");
    agora_native_fini();
    return -1;
  }

  if (!localIP.empty()) {
    std::string ip;
    getLocalIP(connection, ip);
    AG_LOG(INFO, "Local IP:%s", ip.c_str());
  }

  g_initialized = true;
  return 0;
}

void agora_native_fini(void)
{
  if (!g_initialized) {
    return;
  }

  // 修复：添加空指针检查
  if (connection) {
    // Unpublish tracks
    if (customAudioTrack) {
      connection->getLocalUser()->unpublishAudio(customAudioTrack);
    }
    if (customVideoTrack) {
      connection->getLocalUser()->unpublishVideo(customVideoTrack);
    }

    // Unregister observers
    connection->unregisterObserver(connObserver.get());
    connection->unregisterNetworkObserver(connObserver.get());

    // Disconnect
    if (connection->disconnect()) {
      AG_LOG(ERROR, "Failed to disconnect from Agora channel!");
    } else {
      AG_LOG(INFO, "Disconnected from Agora channel successfully");
    }
  }

  // Release resources
  customAudioTrack = nullptr;
  customVideoTrack = nullptr;
  audioFrameSender = nullptr;
  videoFrameSender = nullptr;
  factory = nullptr;
  localUserObserver.reset();
  connObserver.reset();
  connection = nullptr;

  if (service) {
    service->release();
    service = nullptr;
  }

  g_initialized = false;
}

int agora_native_send_video_data(uint8_t *data, size_t len, bool isKeyFrame, VideoCodecType codec_type)
{
  if (!g_initialized || !videoFrameSender) {
    AG_LOG(ERROR, "Agora service not initialized or video sender not available");
    return -1;
  }

  agora::rtc::EncodedVideoFrameInfo videoEncodedFrameInfo;
  videoEncodedFrameInfo.rotation = agora::rtc::VIDEO_ORIENTATION_0;
  videoEncodedFrameInfo.codecType = (codec_type == VideoCodecTypeH265)
      ? agora::rtc::VIDEO_CODEC_H265
      : agora::rtc::VIDEO_CODEC_H264;
  videoEncodedFrameInfo.framesPerSecond = g_video_fps;
  videoEncodedFrameInfo.frameType =
      (isKeyFrame ? agora::rtc::VIDEO_FRAME_TYPE::VIDEO_FRAME_TYPE_KEY_FRAME
                  : agora::rtc::VIDEO_FRAME_TYPE::VIDEO_FRAME_TYPE_DELTA_FRAME);

  // 修复：检查发送结果
  bool success = videoFrameSender->sendEncodedVideoImage(data, len, videoEncodedFrameInfo);
  if (!success) {
    AG_LOG(ERROR, "Failed to send video data: error code %d", success);
  }

  return success ? 0 : -1;
}