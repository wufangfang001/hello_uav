//
//  Agora SDK
//
//  Copyright (c) 2021 Agora.io. All rights reserved.
//

#pragma once  // NOLINT(build/header_guard)

#include "AgoraBase.h"
#include "IAgoraLog.h"
#include "AgoraRefPtr.h"
#include "NGIAgoraVideoFrame.h"
#include "AgoraMediaBase.h"

namespace agora {
namespace rtc {

// class ExtensionControlImpl;
class IAgoraVideoQualityAnalyzer : public RefCountInterface {
 public:
  class Control : public RefCountInterface {
  public:
    virtual void printLog(commons::LOG_LEVEL level, const char* format, ...) = 0;
  };

  virtual ~IAgoraVideoQualityAnalyzer() = default;

  virtual int initializeVQA(const agora_refptr<Control>& control) = 0;

  virtual int pushYuvData(agora::agora_refptr<rtc::IVideoFrame> frame, int fps, int64_t ts) = 0;

  virtual int getVqaResult(float& mos, float* regression_feature, size_t feature_size) = 0;
};


}  // namespace rtc
}  // namespace agora
