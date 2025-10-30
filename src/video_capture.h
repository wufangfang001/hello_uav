#ifndef VIDEO_CAPTURE_H_
#define VIDEO_CAPTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer {
    void *data;
    unsigned int length;
} Buffer;

int capture_init();
int camera_capture_start();

Buffer get_one_frame();
void clear_one_frame();
void capture_fini();

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VIDEO_CAPTURE_H_ */