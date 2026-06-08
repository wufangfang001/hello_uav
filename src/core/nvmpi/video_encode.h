#ifndef VIDEO_ENCODE_H_
#define VIDEO_ENCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "agora_config.h"

int video_encode_init(VideoCodecType codec, uint32_t width, uint32_t height, uint32_t fps, uint32_t bps);
void video_encode_fini(void);
int video_encode_encoding(uint8_t *yuv420, uint8_t **out, bool *is_key_frame);
int video_encode_generate_key_frame(void);
int video_encode_set_target_bps(int bps);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VIDEO_ENCODE_H_ */
