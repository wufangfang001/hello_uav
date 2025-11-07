
//  Agora RTC/MEDIA SDK
//
//  Created by Jay Zhang in 2020-06.
//  Copyright (c) 2019 Agora.io. All rights reserved.
//

#include "sample_common.h"

#include "log.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "AgoraBase.h"
#include "NGIAgoraRtcConnection.h"
#include "NGIAgoraLocalUser.h"

#if defined(TARGET_OS_LINUX)
#define DEFAULT_LOG_PATH ("./io.agora.rtc_sdk/agorasdk.log")
#else
#define DEFAULT_LOG_PATH ("./log/native/agorasdk.log")
#endif

#define DEFAULT_LOG_SIZE (512 * 1024)  // default log size is 512 kb

#define INTERNAL_RTC_KEY_LOCAL_IP                             "rtc.local.ip"
#define INTERNAL_RTC_KEY_RTC_UDP_SEND_FD                      "rtc.udp_send_fd"

// @WARNING : IAgoraService is Global singleton ！！Just create it once and make sure you don't destroy it before process end.
// @WARNING : Be careful when you Mult_thread programming！！
agora::base::IAgoraService* createAndInitAgoraService(bool enableAudioDevice,
                                                      bool enableAudioProcessor, bool enableVideo,bool enableuseStringUid,bool enablelowDelay,const char* appid) {
  int32_t buildNum = 0;
  getAgoraSdkVersion(&buildNum);
#if defined(SDK_BUILD_NUM)
  if ( buildNum != SDK_BUILD_NUM ) {
    AG_LOG(ERROR, "SDK VERSION CHECK FAILED!\nSDK version: %d\nAPI Version: %d\n", buildNum, SDK_BUILD_NUM);
    //return nullptr;
  }
#endif
  AG_LOG(INFO, "SDK version: %d\n", buildNum);
  auto service = createAgoraService();
  agora::base::AgoraServiceConfiguration scfg;
  scfg.appId = appid;
  scfg.enableAudioProcessor = enableAudioProcessor;
  scfg.enableAudioDevice = enableAudioDevice;
  scfg.enableVideo = enableVideo;
  scfg.useStringUid = enableuseStringUid;
  if(enablelowDelay){
    scfg.channelProfile = agora::CHANNEL_PROFILE_TYPE::CHANNEL_PROFILE_CLOUD_GAMING;
  }
  if (service->initialize(scfg) != agora::ERR_OK) {
    return nullptr;
  }

  AG_LOG(INFO, "Created log file at %s", DEFAULT_LOG_PATH);
  if (service->setLogFile(DEFAULT_LOG_PATH, DEFAULT_LOG_SIZE) != 0) {
    return nullptr;
  }
 if(verifyLicense() != 0) return nullptr;
  return service;
}

class LicenseCallbackImpl : public agora::base::LicenseCallback
{
public:
  LicenseCallbackImpl() {}
  virtual ~LicenseCallbackImpl() {}

  virtual void onCertificateRequired() {
    AG_LOG(INFO, "[%s, line: %d]", __FUNCTION__, __LINE__);
  }

  virtual void onLicenseRequest() {
    AG_LOG(INFO, "[%s, line: %d]", __FUNCTION__, __LINE__);
  }

  virtual void onLicenseValidated() {
    AG_LOG(INFO, "[%s, line: %d]", __FUNCTION__, __LINE__);
  }

  virtual void onLicenseError(int result) {
    AG_LOG(ERROR, "[%s, line: %d] result: %d", __FUNCTION__, __LINE__, result);
  }
};

static const std::string CERTIFICATE_FILE = "certificate.bin";

int verifyLicense()
{
#ifdef LICENSE_CHECK
  // Step1: read the certificate buffer from the certificate.bin file
  char* cert_buffer = NULL;
  int cert_length = 0;
  std::ifstream f_cert(CERTIFICATE_FILE.c_str(), std::ios::binary);
  if (f_cert) {
    f_cert.seekg(0, f_cert.end);
    cert_length = f_cert.tellg();
    f_cert.seekg(0, f_cert.beg);

    cert_buffer = new char[cert_length + 1];
    AG_LOG(INFO, "cert_length: %d", cert_length);
    memset(cert_buffer, 0, cert_length + 1);
    f_cert.read(cert_buffer, cert_length);
    if (!cert_buffer || f_cert.gcount() < cert_length) {
      f_cert.close();
      if (cert_buffer) {
        delete[] cert_buffer;
        cert_buffer = NULL;
      }
      AG_LOG(ERROR, "read %s failed", CERTIFICATE_FILE.c_str());
      return -1;
    }
    else {
      f_cert.close();
    }
  }
  else {
    AG_LOG(ERROR, "%s doesn't exist", CERTIFICATE_FILE.c_str());
    return -1;
  }
  AG_LOG(INFO, "certificate: %s", cert_buffer);

  // Step3: register callback of license state
  LicenseCallbackImpl *cb = static_cast<LicenseCallbackImpl *>(getAgoraLicenseCallback());
  if (!cb) {
    cb = new LicenseCallbackImpl();
    setAgoraLicenseCallback(static_cast<agora::base::LicenseCallback *>(cb));
  }

  // Step4: verify the license with credential and certificate
  int result = getAgoraCertificateVerifyResult(NULL, 0, cert_buffer, cert_length);
  AG_LOG(INFO, "verify result: %d", result);

  if (cert_buffer) {
    delete[] cert_buffer;
    cert_buffer = NULL;
  }

  return result;
#else
  return 0;
#endif
}

int32_t getLocalIP(agora::agora_refptr<agora::rtc::IRtcConnection>& connection, std::string& ip) {
  int32_t retIntValue = -1;
  const char* intKey = INTERNAL_RTC_KEY_RTC_UDP_SEND_FD;
  agora::base::IAgoraParameter* agoraParameter = connection->getAgoraParameter();
  agoraParameter->getInt(intKey, retIntValue);
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  if (getsockname(retIntValue, (struct sockaddr *)&addr, &len)) {
    perror("getsockname");
    return -1;
  }
  ip = std::string(inet_ntoa(addr.sin_addr));
  AG_LOG(INFO, "getLocalIP:%s", ip.c_str());
  return 0;
}

int32_t setLocalIP(agora::agora_refptr<agora::rtc::IRtcConnection>& connection, const std::string& ip) {
  agora::util::AString retStringValue;
  const char* stringKey = INTERNAL_RTC_KEY_LOCAL_IP;
  agora::base::IAgoraParameter* agoraParameter = connection->getAgoraParameter();
  agoraParameter->setString(stringKey, ip.c_str());
  agoraParameter->getString(stringKey, retStringValue);
  AG_LOG(INFO, "setLocalIP:%s", retStringValue->data());
  return 0;
}

int32_t setLowDelay(agora::agora_refptr<agora::rtc::IRtcConnection> &connection)
{
	auto s = connection->getAgoraParameter();
	int32_t ret = 0;
  connection->getLocalUser()->setAudioScenario(agora::rtc::AUDIO_SCENARIO_TYPE::AUDIO_SCENARIO_CHORUS);
	ret = s->setUInt("che.audio.uplink_max_retry_times", 5);
	if (!ret) {
		AG_LOG(INFO, "[Low Delay] set che.audio.uplink_max_retry_times 5 successfully!");
	} else {
		AG_LOG(INFO, "[Low Delay] set che.audio.uplink_max_retry_times 5  fail!!! The err num is %d",ret);
	}

	ret = s->setUInt("che.audio.downlink_max_retry_times", 5);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set che.audio.downlink_max_retry_times 5 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set che.audio.downlink_max_retry_times 5  fail!!! The err num is %d",ret);
  } 

	ret = s->setUInt("rtc.video.downMaxRetryTimes", 5);
	if (!ret) {
		AG_LOG(INFO, "[Low Delay] set rtc.video.downMaxRetryTimes 5 successfully!");
	} else {
		AG_LOG(INFO, "[Low Delay] set rtc.video.downMaxRetryTimes 5  fail!!! The err num is %d",ret);
	}

  ret = s->setInt("rtc.paced_sender_enabled", 0);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.paced_sender_enabled 0 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.paced_sender_enabled fail! The err num is %d", ret);
  }

  ret = s->setInt("rtc.video.playout_delay_min", 0);
	if (!ret) {
		AG_LOG(INFO, "[Low Delay] set rtc.video.playout_delay_min 0 successfully!");
	} else {
		AG_LOG(INFO, "[Low Delay] set rtc.video.playout_delay_min 0 fail!!! The err num is %d", ret);
	}

  ret = s->setBool("che.video.vpr.enable", false);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set che.video.vpr.enable false successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set che.video.vpr.enable false fail!!! The err num is %d", ret);
  }

  ret = s->setBool("rtc.video.avsync", false);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.avsync false successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.video.avsync false fail!!! The err num is %d", ret);
  }

  ret = s->setUInt("che.video.harqScene", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set che.video.harqScene 1 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set che.video.harqScene 1 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("che.video.fec_outside_bw_ratio", 20);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set che.video.fec_outside_bw_ratio 20 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set che.video.fec_outside_bw_ratio 20 fail!!! The err num is %d", ret);
  }

  ret = s->setBool("rtc.video.apas_harq_enable", true);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.apas_harq_enable true successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.video.apas_harq_enable true fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.ack_delay", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.ack_delay 1 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.ack_delay 1 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.remote_ack_delay", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.remote_ack_delay 1 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.remote_ack_delay 1 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.video.old_render_timestamp_gap", 200);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.old_render_timestamp_gap 200 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.video.old_render_timestamp_gap 200 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.cc_private", 1048);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.cc_private 1048 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.cc_private 1048 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.remote_cc_private", 1048);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.remote_cc_private 1048 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.remote_cc_private 1048 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.congestion_window_compensation_mode", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.congestion_window_compensation_mode 1 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.congestion_window_compensation_mode 1 fail!!! The err num is %d", ret);
  }

  ret = s->setBool("rtc.video.decoder_out_byte_frame", false);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.decoder_out_byte_frame false successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.video.decoder_out_byte_frame false fail!!! The err num is %d", ret);
  }

  ret = s->setString("che.video.broadcast.special_config", "{\"che.video.vpr.enable\": false}");
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set che.video.broadcast.special_config successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set che.video.broadcast.special_config fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.video.broadcaster_playout_delay_max", 0);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.broadcaster_playout_delay_max 0 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.video.broadcaster_playout_delay_max 0 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.video.playout_delay_max", 0);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.playout_delay_max 0 successfully!");
  } else {
    AG_LOG(INFO, "[Low Delay] set rtc.video.playout_delay_max 0 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.remote_congestion_window_compensation_mode", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.remote_congestion_window_compensation_mode 1 successfully!");
  } else {  
    AG_LOG(INFO, "[Low Delay] set rtc.remote_congestion_window_compensation_mode 1 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.enable_conservative_probe_rtt_mode", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.enable_conservative_probe_rtt_mode 1 successfully!");
  } else {  
    AG_LOG(INFO, "[Low Delay] set rtc.enable_conservative_probe_rtt_mode 1 fail!!! The err num is %d", ret);
  }

  ret = s->setInt("rtc.remote_enable_conservative_probe_rtt_mode", 1);
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.remote_enable_conservative_probe_rtt_mode 1 successfully!");
  } else {  
    AG_LOG(INFO, "[Low Delay] set rtc.remote_enable_conservative_probe_rtt_mode 1 fail!!! The err num is %d", ret);
  }

  ret = s->setString("rtc.video.enable_sr", " {\"enabled\": false, \"mode\":2}");
  if (!ret) {
    AG_LOG(INFO, "[Low Delay] set rtc.video.enable_sr successfully!");
  } else {  
    AG_LOG(INFO, "[Low Delay] set rtc.video.enable_sr fail!!! The err num is %d", ret);
  }

  return ret;
}
