#ifndef INC_VIDEO_CONFIG_H_
#define INC_VIDEO_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void (* f_sdk_target_bitrate_change)(uint32_t target_bps);
typedef void (* f_sdk_key_frame_request)(void);
typedef void (* f_sdk_should_stop)(void);

typedef struct {
  char appid[64];
  char token[512];
  char channel[64];

  uint32_t video_width;
  uint32_t video_height;
  uint32_t video_bps;
  uint32_t video_fps;

  bool b_enable_multi_path;
  bool b_enable_rtsa;

  f_sdk_target_bitrate_change f_bps_change;
  f_sdk_key_frame_request     f_key_frame;
  f_sdk_should_stop           f_stop;
} agora_config_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INC_VIDEO_CONFIG_H_ */
