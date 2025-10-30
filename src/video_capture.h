#ifndef VIDEO_CAPTURE_H_
#define VIDEO_CAPTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer {
    void *data;
    unsigned int length;
} Buffer;

int video_capture_init();
int video_capture_start();

Buffer video_capture_try_get_one_frame();
void video_capture_clear_one_frame();
void video_capture_fini();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VIDEO_CAPTURE_H_ */