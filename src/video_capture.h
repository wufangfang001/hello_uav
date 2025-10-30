#ifndef VIDEO_CAPTURE_H_
#define VIDEO_CAPTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int video_capture_init();
int video_capture_start();

int video_capture_try_get_one_frame(uint8_t **data);
void video_capture_clear_one_frame();

int video_capture_stop();
void video_capture_fini();

void yuyv2yuv420(uint8_t *yuyv, uint8_t *yuv420, int width, int height);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VIDEO_CAPTURE_H_ */