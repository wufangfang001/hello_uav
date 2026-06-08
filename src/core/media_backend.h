#ifndef MEDIA_BACKEND_H_
#define MEDIA_BACKEND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "media_pipeline.h"

typedef struct {
  const char *name;
  int (*init)(const media_video_config_t *config);
  int (*start)(void);
  int (*acquire_frame)(media_encoded_frame_t *frame);
  int (*release_frame)(media_encoded_frame_t *frame);
  int (*request_key_frame)(void);
  int (*set_target_bps)(uint32_t target_bps);
  int (*stop)(void);
  void (*fini)(void);
} media_backend_ops_t;

const media_backend_ops_t *media_backend_get_ops(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MEDIA_BACKEND_H_ */
