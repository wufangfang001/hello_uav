#ifndef MEDIA_PIPELINE_H_
#define MEDIA_PIPELINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "agora_config.h"

typedef struct {
  VideoCodecType codec_type;
  const char *device_name;
  uint32_t width;
  uint32_t height;
  uint32_t fps;
  uint32_t capture_fps;
  uint32_t bps;
  uint32_t device_id;
  uint32_t pipe_id;
  uint32_t channel_id;
} media_video_config_t;

typedef struct {
  uint8_t *data;
  size_t len;
  bool is_key_frame;
  void *priv;
} media_encoded_frame_t;

int media_pipeline_init(const media_video_config_t *config);
int media_pipeline_start(void);
int media_pipeline_acquire_frame(media_encoded_frame_t *frame);
int media_pipeline_release_frame(media_encoded_frame_t *frame);
int media_pipeline_request_key_frame(void);
int media_pipeline_set_target_bps(uint32_t target_bps);
int media_pipeline_stop(void);
void media_pipeline_fini(void);

const char *media_pipeline_backend_name(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MEDIA_PIPELINE_H_ */
