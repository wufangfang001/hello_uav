#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "agora_log.h"
#include "video_config.h"
#include "video_capture.h"

#define TAG  "[CAP]"

typedef struct buffer {
  void *data;
  unsigned int length;
} Buffer;

static int                g_camera_fd  = -1;
static Buffer*            g_buffers    = NULL;
static int                g_buffer_num = 4;
static struct v4l2_buffer g_v4l2_buf   = { 0 };

#define DEVICE_NAME "/dev/video0"

static int __camera_ioctl(int request, void *arg)
{
  int ret;

  do {
    ret = ioctl(g_camera_fd, request, arg);
  } while (ret < 0 && EINTR == errno);

  return ret;
}

int video_capture_start()
{
  int i = 0;
  struct v4l2_buffer v4l2_buf = { 0 };
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  v4l2_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_buf.memory = V4L2_MEMORY_MMAP;

  for (i = 0; i < g_buffer_num; ++i) {
    v4l2_buf.index = i;
    if (0 > __camera_ioctl(VIDIOC_QBUF, &v4l2_buf)) {
      LOGE(TAG, "VIDIOC_QBUF error:%d %s", errno, strerror(errno));
      return -1;
    }
  }

  if (0 > __camera_ioctl(VIDIOC_STREAMON, &type)) {
    LOGE(TAG, "VIDIOC_STREAMON error:%d %s", errno, strerror(errno));
    return -1;
  }

  return 0;
}

static int __camera_set_fps()
{
  struct v4l2_streamparm setfps = { 0 };
  setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (0 > __camera_ioctl(VIDIOC_G_PARM, &setfps)) {
    LOGE(TAG, "VIDIOC_G_PARM error: %d %s", errno, strerror(errno));
    return -1;
  }

  setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  setfps.parm.capture.timeperframe.numerator   = 1;
  setfps.parm.capture.timeperframe.denominator = CAPTURE_FPS;
  if (0 > __camera_ioctl(VIDIOC_S_PARM, &setfps)) {
    LOGE(TAG, "VIDIOC_S_PARM error: %d %s", errno, strerror(errno));
    return -1;
  }

  return 0;
}

int video_capture_try_get_one_frame(uint8_t **data)
{
  struct v4l2_buffer v4l2_buf = { 0 };

  v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_buf.memory = V4L2_MEMORY_MMAP;

  // select等待数据
  fd_set rset;
  FD_ZERO(&rset);
  FD_SET(g_camera_fd, &rset);
  struct timeval tv = {1, 0};
  
  int retsel = select(g_camera_fd + 1, &rset, NULL, NULL, &tv);
  if (retsel <= 0) {
    LOGE(TAG, "select failed, retsel=%d, errno=%d, %s", retsel, errno, strerror(errno));
    return 0;
  }

  // 获取缓冲区
  if (0 > __camera_ioctl(VIDIOC_DQBUF, &v4l2_buf)) {
    LOGE(TAG, "VIDIOC_DQBUF error: %d %s", errno, strerror(errno));
    return 0;
  }

  // 更新全局状态
  g_v4l2_buf = v4l2_buf;

  *data = g_buffers[v4l2_buf.index].data;
  return v4l2_buf.length;
}

void video_capture_clear_one_frame()
{
  if (g_camera_fd < 0) {
    return;
  }

  if (0 > __camera_ioctl(VIDIOC_QBUF, &g_v4l2_buf)) {
    LOGE(TAG, "camera_read_frame VIDIOC_QBUF error:%d %s", errno, strerror(errno));
  }
}

static int __camera_open()
{
  struct stat devStat = { 0 };

  if (0 > stat(DEVICE_NAME, &devStat)) {
    LOGE(TAG, "get device[%s] info failed: %d, %s", DEVICE_NAME, errno, strerror(errno));
    return -1;
  }

  if (!S_ISCHR(devStat.st_mode)) {
    LOGE(TAG, "%s is not char device!", DEVICE_NAME);
    return -1;
  }

  if (0 > (g_camera_fd = open(DEVICE_NAME, O_RDWR, 0))) {
    LOGE(TAG, "cannot open %s:%d, %s", DEVICE_NAME, errno, strerror(errno));
    return -1;
  }

  return 0;
}

static int __camera_query_cap()
{
  struct v4l2_capability cap = { 0 };

  if (0 > __camera_ioctl(VIDIOC_QUERYCAP, &cap)) {
    LOGE(TAG, "VIDIOC_QUERYCAP error: %d %s", errno, strerror(errno));
    return -1;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    LOGE(TAG, "device does not support capture!!!");
    return -1;
  }

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
    LOGE(TAG, "device does not support streaming!!!");
    return -1;
  }

  LOGT(TAG, "VIDIOC_QUERYCAP");
  LOGT(TAG, "driver:%s", cap.driver);
  LOGT(TAG, "card:%s", cap.card);
  LOGT(TAG, "bus info:%s", cap.bus_info);
  LOGT(TAG, "version:%u", cap.version);
  LOGT(TAG, "capabilities:%x", cap.capabilities);

  struct v4l2_fmtdesc dis_fmtdesc;
  dis_fmtdesc.index = 0;
  dis_fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  LOGT(TAG, "Support format:");

  while (ioctl(g_camera_fd, VIDIOC_ENUM_FMT, &dis_fmtdesc) != -1) {
    printf("\t%d.%s  %d\n", dis_fmtdesc.index + 1, dis_fmtdesc.description, dis_fmtdesc.type);
    dis_fmtdesc.index++;
  }

  return 0;
}

static int __camera_set_video_fmt()
{
  struct v4l2_format fmt = { 0 };

  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = CAPTURE_WIDTH;
  fmt.fmt.pix.height      = CAPTURE_HEIGHT;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

  if (0 > __camera_ioctl(VIDIOC_S_FMT, &fmt)) {
    LOGE(TAG, "VIDIOC_S_FMT error:%d %s", errno, strerror(errno));
    return -1;
  }

  LOGT(TAG, "VIDIOC_S_FMT:%d*%d %d", fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat);
  return 0;
}

static int __camera_buffer_release(int num)
{
  int i = 0;

  if (NULL == g_buffers) {
    return 0;
  }

  for (i = 0; i < num; ++i) {
    if (g_buffers[i].data != NULL && g_buffers[i].data != MAP_FAILED) {
      munmap(g_buffers[i].data, g_buffers[i].length);
      g_buffers[i].data = NULL;
    }
  }

  free(g_buffers);
  g_buffers = NULL;
  return 0;
}

static int __camera_request_buffer()
{
  struct v4l2_requestbuffers req = { 0 };
  struct v4l2_buffer v4l2Buf = { 0 };
  int i = 0;

  req.count  = 4;  // 内核空间内存，申请4个帧缓冲空间
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;  // 使用mmap

  if (0 > __camera_ioctl(VIDIOC_REQBUFS, &req)) {
    LOGE(TAG, "VIDIOC_REQBUFS error:%d %s", errno, strerror(errno));
    return -1;
  }

  LOGT(TAG, "VIDIOC_REQBUFS:count=%d", req.count);
  g_buffer_num = req.count;

  v4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2Buf.memory = V4L2_MEMORY_MMAP;

  if (NULL == (g_buffers = (Buffer *)calloc(req.count, sizeof(*(g_buffers))))) {
    LOGT(TAG, "calloc failed,Out of memory");
    return -1;
  }

  for (i = 0; i < req.count; ++i) {
    v4l2Buf.index = i;
    if (0 > __camera_ioctl(VIDIOC_QUERYBUF, &v4l2Buf)) {
      LOGE(TAG, "VIDIOC_QUERYBUF [%d] error: %d %s", i, errno, strerror(errno));
      return -1;
    }

    g_buffers[i].length = v4l2Buf.length;
    g_buffers[i].data   = mmap(NULL, v4l2Buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, g_camera_fd, v4l2Buf.m.offset);
    if (MAP_FAILED == g_buffers[i].data) {
      LOGT(TAG, "buffer[%d] mmap failed", i);
      if (i > 0) {
        __camera_buffer_release(i);
      }
      return -1;
    }
  }

  return 0;
}

void yuyv2yuv420(uint8_t *yuyv, uint8_t *yuv420, int width, int height)
{
  int pixs = width * height;
  uint8_t *y = yuv420;
  uint8_t *u = yuv420 + pixs;
  uint8_t *v = yuv420 + pixs + (pixs >> 2);
  uint8_t *start = yuyv;

  /*处理Y分量*/
  for (int j = 0; j < pixs * 2; j = j + 2) {
    *y++ = *(start + j);
  }

  /**处理UV分量**/
  start = yuyv;
  for (int h = 0; h < height; h += 2) { // 隔行, 我选择保留偶数行
    for (int w = h * width * 2 + 1; w < width * 2 * (h + 1); w += 4) { // YUYV单行中每四个字节含有一对UV分量
      *u++ = *(start + w);
      *v++ = *(start + w + 2);
    }
  }
}

int video_capture_init()
{
  int ret;

  if (0 != (ret = __camera_open())) {
    LOGE(TAG, "camera_open error!");
    return ret;
  }

  if (0 != (ret = __camera_query_cap())) {
    LOGE(TAG, "camera_query_cap error!");
    return ret;
  }

  if (0 != (ret = __camera_set_video_fmt())) {
    LOGE(TAG, "camera_set_video_fmt error!");
    return ret;
  }

  if (0 != (ret = __camera_set_fps())) {
    LOGE(TAG, "camera_set_fps error!");
    return ret;
  }

  if (0 != (ret = __camera_request_buffer())) {
    LOGE(TAG, "camera_request_buffer error!");
    return ret;
  }

  return ret;
}

int video_capture_stop()
{
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  // 停止视频流
  if (g_camera_fd >= 0) {
    if (0 > __camera_ioctl(VIDIOC_STREAMOFF, &type)) {
      LOGE(TAG, "VIDIOC_STREAMOFF error:%d %s", errno, strerror(errno));
      return -1;
    }
  }

  return 0;
}

void video_capture_fini()
{
  // 释放映射的缓冲区
  if (g_buffers != NULL) {
    __camera_buffer_release(g_buffer_num);
  }

  // 关闭设备文件
  if (g_camera_fd >= 0) {
    close(g_camera_fd);
    g_camera_fd = -1;
  }
}