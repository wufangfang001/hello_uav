#ifndef H264_ENCODE_H_
#define H264_ENCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int h264_encode_init();
void h264_encode_fini();
int h264_encode_encoding(uint8_t *yuv420, uint8_t **h264);
int h264_encode_generate_key_frame();
int h264_encode_set_target_bps(int bps);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* H264_ENCODE_H_ */