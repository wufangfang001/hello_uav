#include <string.h>

#include "h264_encode.h"
#include "agora_log.h"
#include "video_config.h"
#include "nvmpi.h"

#define TAG "[codec]"

#define H264_DATA_BUFFER_LEN (128 * 1024)

static nvmpictx* g_ctx         = NULL;
static int       g_current_bps = VIDEO_ENCODE_TARGET_BPS;
static uint8_t*  g_h264_buffer = NULL;
static int       g_buffer_len  = H264_DATA_BUFFER_LEN;

int h264_encode_init()
{
  nvEncParam param = { 0 };
  param.width             = CAPTURE_WIDTH;
  param.height            = CAPTURE_HEIGHT;
  param.bitrate           = VIDEO_ENCODE_TARGET_BPS;
  param.iframe_interval   = CAPTURE_FPS * 2;
  param.fps_n             = CAPTURE_FPS;
  param.fps_d             = 1;
  param.insert_spspps_idr = 1;
  param.profile           = 1;
  param.level             = 41;
  param.capture_num       = 4;

  if (NULL == (g_ctx = nvmpi_create_encoder(NV_VIDEO_CodingH264, &param))) {
    LOGE(TAG, "Failed to create encoder");
    return -1;
  }

  g_h264_buffer = (uint8_t *)malloc(g_buffer_len);
  g_current_bps = VIDEO_ENCODE_TARGET_BPS;
  LOGT(TAG, "nvmpi_create_encoder success.");
  return 0;
}

int h264_encode_encoding(uint8_t *yuv420, uint8_t **h264)
{
  nvFrame frame   = { 0 };
  nvPacket packet = { 0 };
  int total_len = 0;

  *h264 = NULL;

  size_t y_size  = CAPTURE_WIDTH * CAPTURE_HEIGHT;
  size_t uv_size = y_size / 4;

  frame.type   = NV_PIX_YUV420;
  frame.width  = CAPTURE_WIDTH;
  frame.height = CAPTURE_HEIGHT;

  frame.payload[0] = yuv420;
  frame.payload[1] = yuv420 + y_size;
  frame.payload[2] = yuv420 + y_size + uv_size;

  frame.payload_size[0] = y_size;
  frame.payload_size[1] = uv_size;
  frame.payload_size[2] = uv_size;

  if (nvmpi_encoder_put_frame(g_ctx, &frame) < 0) {
    LOGE(TAG, "put_frame failed");
    return -1;
  }

  while (1) {
    memset(&packet, 0, sizeof(packet));
    if (0 != nvmpi_encoder_get_packet(g_ctx, &packet)) {
      break;
    }

    if (total_len >= g_buffer_len) {
      g_buffer_len *= 2;
      g_h264_buffer = (uint8_t *)realloc(g_h264_buffer, g_buffer_len);
    }

    memcpy(g_h264_buffer + total_len, packet.payload, packet.payload_size);
    total_len += packet.payload_size;
  }

  *h264 = g_h264_buffer;
  return total_len;
}

int h264_encode_generate_key_frame()
{
  if (NULL == g_ctx) {
    LOGE(TAG, "g_ctx null");
    return -1;
  }

  if (0 > nvmpi_encoder_force_idr(g_ctx)) {
    LOGE(TAG, "nvmpi_encoder_force_idr error.");
    return -1;
  }

  return 0;
}

#define __MIN(a, b) (a < b ? a : b)
int h264_encode_set_target_bps(int bps)
{
  if (NULL == g_ctx) {
    LOGE(TAG, "g_ctx null");
    return -1;
  }

  bps = __MIN(bps, VIDEO_ENCODE_TARGET_BPS);
  if (g_current_bps == bps) {
    return 0;
  }

  if (0 > nvmpi_encoder_setBitrate(g_ctx, g_current_bps)) {
    LOGE(TAG, "nvmpi_encoder_setBitrate error.");
    return -1;
  }

  g_current_bps = bps;
  LOGT(TAG, "nvmpi_encoder_setBitrate[%dKbps] success", g_current_bps / 1000);
  return 0;
}

void h264_encode_fini()
{
  nvmpi_encoder_close(g_ctx);
  g_ctx = NULL;

  if (g_h264_buffer) {
    free(g_h264_buffer);
    g_h264_buffer = NULL;
  }

  LOGT(TAG, "h264_encode_fini success.");
}