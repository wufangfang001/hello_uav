#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "agora_log.h"
#include "media_backend.h"
#include "video_capture.h"
#include "video_encode.h"

#define TAG "[media-nvmpi]"

static media_video_config_t g_config = { 0 };
static uint8_t *g_yuv420 = NULL;
static size_t g_yuv420_len = 0;
static bool g_capture_initialized = false;
static bool g_encode_initialized = false;
static bool g_started = false;
static bool g_frame_held = false;

static int __nvmpi_release_capture_frame(void)
{
  if (!g_frame_held) {
    return 0;
  }

  video_capture_clear_one_frame();
  g_frame_held = false;
  return 0;
}

static int __nvmpi_init(const media_video_config_t *config)
{
  memset(&g_config, 0, sizeof(g_config));
  g_config = *config;

  g_yuv420_len = (size_t)config->width * config->height * 3 / 2;
  g_yuv420 = (uint8_t *)malloc(g_yuv420_len);
  if (g_yuv420 == NULL) {
    LOGE(TAG, "failed to allocate yuv420 buffer, len=%zu", g_yuv420_len);
    return -1;
  }

  if (video_encode_init(config->codec_type,
                        config->width,
                        config->height,
                        config->fps,
                        config->bps) != 0) {
    LOGE(TAG, "video encode init failed");
    goto fail;
  }
  g_encode_initialized = true;

  if (video_capture_init(config->width, config->height, config->fps) != 0) {
    LOGE(TAG, "video capture init failed");
    goto fail;
  }
  g_capture_initialized = true;

  return 0;

fail:
  if (g_encode_initialized) {
    video_encode_fini();
    g_encode_initialized = false;
  }

  video_capture_fini();
  g_capture_initialized = false;
  free(g_yuv420);
  g_yuv420 = NULL;
  g_yuv420_len = 0;
  g_started = false;
  g_frame_held = false;
  memset(&g_config, 0, sizeof(g_config));
  return -1;
}

static int __nvmpi_start(void)
{
  if (!g_capture_initialized) {
    LOGE(TAG, "start failed: capture not initialized");
    return -1;
  }

  if (g_started) {
    return 0;
  }

  if (video_capture_start() != 0) {
    LOGE(TAG, "video capture start failed");
    return -1;
  }

  g_started = true;
  return 0;
}

static int __nvmpi_acquire_frame(media_encoded_frame_t *frame)
{
  uint8_t *yuyv = NULL;
  uint8_t *encoded_data = NULL;
  int yuyv_len;
  int encoded_len;
  bool is_key_frame = false;

  if (g_frame_held) {
    LOGE(TAG, "acquire failed: previous frame has not been released");
    return -1;
  }

  yuyv_len = video_capture_try_get_one_frame(&yuyv);
  if (yuyv_len <= 0) {
    return yuyv_len < 0 ? -1 : 0;
  }

  yuyv2yuv420(yuyv, g_yuv420, g_config.width, g_config.height);

  encoded_len = video_encode_encoding(g_yuv420, &encoded_data, &is_key_frame);
  if (encoded_len < 0) {
    video_capture_clear_one_frame();
    return -1;
  }

  if (encoded_len == 0) {
    video_capture_clear_one_frame();
    return 0;
  }

  frame->data = encoded_data;
  frame->len = (size_t)encoded_len;
  frame->is_key_frame = is_key_frame;
  frame->priv = NULL;
  g_frame_held = true;
  return 1;
}

static int __nvmpi_release_frame(media_encoded_frame_t *frame)
{
  (void)frame;
  return __nvmpi_release_capture_frame();
}

static int __nvmpi_request_key_frame(void)
{
  return video_encode_generate_key_frame();
}

static int __nvmpi_set_target_bps(uint32_t target_bps)
{
  return video_encode_set_target_bps((int)target_bps);
}

static int __nvmpi_stop(void)
{
  int ret = 0;

  __nvmpi_release_capture_frame();

  if (g_started) {
    ret = video_capture_stop();
    g_started = false;
  }

  return ret;
}

static void __nvmpi_fini(void)
{
  __nvmpi_stop();

  if (g_capture_initialized) {
    video_capture_fini();
    g_capture_initialized = false;
  }

  if (g_encode_initialized) {
    video_encode_fini();
    g_encode_initialized = false;
  }

  free(g_yuv420);
  g_yuv420 = NULL;
  g_yuv420_len = 0;
  g_started = false;
  g_frame_held = false;
  memset(&g_config, 0, sizeof(g_config));
}

static const media_backend_ops_t g_nvmpi_backend_ops = {
  .name = "nvmpi",
  .init = __nvmpi_init,
  .start = __nvmpi_start,
  .acquire_frame = __nvmpi_acquire_frame,
  .release_frame = __nvmpi_release_frame,
  .request_key_frame = __nvmpi_request_key_frame,
  .set_target_bps = __nvmpi_set_target_bps,
  .stop = __nvmpi_stop,
  .fini = __nvmpi_fini,
};

const media_backend_ops_t *media_backend_get_ops(void)
{
  return &g_nvmpi_backend_ops;
}
