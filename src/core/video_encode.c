#include <string.h>

#include "video_encode.h"
#include "agora_log.h"
#include "nvmpi.h"

#define TAG "[codec]"

#define VIDEO_DATA_BUFFER_LEN (128 * 1024)

static nvmpictx* g_ctx         = NULL;
static uint32_t  g_target_bps  = 1500000;
static uint32_t  g_current_bps = 1500000;
static uint32_t  g_width       = 1280;
static uint32_t  g_height      = 720;
static uint8_t*  g_buffer      = NULL;
static int       g_buffer_len  = VIDEO_DATA_BUFFER_LEN;
static const char* g_codec_name = "none"; // h264/h265

int video_encode_init(VideoCodecType codec, uint32_t width, uint32_t height, uint32_t fps, uint32_t bps)
{
  bool is_h265 = (codec == VideoCodecTypeH265);
  g_codec_name = is_h265 ? "h265" : "h264";

  nvCodingType coding = is_h265 ? NV_VIDEO_CodingHEVC : NV_VIDEO_CodingH264;

  nvEncParam param = { 0 };
  param.width             = width;
  param.height            = height;
  param.bitrate           = bps;
  param.iframe_interval   = fps * 2;
  param.fps_n             = fps;
  param.fps_d             = 1;
  param.insert_spspps_idr = 1;
  param.profile           = 1;
  param.level             = is_h265 ? 150 : 41;
  param.capture_num       = 4;

  if (NULL == (g_ctx = nvmpi_create_encoder(coding, &param))) {
    LOGE(TAG, "Failed to create %s encoder", g_codec_name);
    return -1;
  }

  g_buffer      = (uint8_t *)malloc(g_buffer_len);
  g_current_bps = bps;
  g_target_bps  = bps;
  g_width       = width;
  g_height      = height;
  LOGT(TAG, "%s encode init success.", g_codec_name);
  return 0;
}

int video_encode_encoding(uint8_t *yuv420, uint8_t **out, bool *is_key_frame)
{
  nvFrame frame   = { 0 };
  nvPacket packet = { 0 };
  int total_len = 0;

  *out = NULL;

  size_t y_size  = g_width * g_height;
  size_t uv_size = y_size / 4;

  frame.type   = NV_PIX_YUV420;
  frame.width  = g_width;
  frame.height = g_height;

  frame.payload[0] = yuv420;
  frame.payload[1] = yuv420 + y_size;
  frame.payload[2] = yuv420 + y_size + uv_size;

  frame.payload_size[0] = y_size;
  frame.payload_size[1] = uv_size;
  frame.payload_size[2] = uv_size;

  if (nvmpi_encoder_put_frame(g_ctx, &frame) < 0) {
    LOGE(TAG, "%s put_frame failed", g_codec_name);
    return -1;
  }

  while (1) {
    memset(&packet, 0, sizeof(packet));
    if (0 != nvmpi_encoder_get_packet(g_ctx, &packet)) {
      break;
    }

    if (total_len >= g_buffer_len) {
      g_buffer_len *= 2;
      g_buffer = (uint8_t *)realloc(g_buffer, g_buffer_len);
    }

    memcpy(g_buffer + total_len, packet.payload, packet.payload_size);
    total_len += packet.payload_size;
    *is_key_frame = (packet.flags & 0x00000001) != 0;
  }

  *out = g_buffer;
  return total_len;
}

int video_encode_generate_key_frame(void)
{
  if (NULL == g_ctx) {
    LOGE(TAG, "%s g_ctx null", g_codec_name);
    return -1;
  }

  if (0 > nvmpi_encoder_force_idr(g_ctx)) {
    LOGE(TAG, "%s nvmpi_encoder_force_idr error.", g_codec_name);
    return -1;
  }

  return 0;
}

#define __MIN(a, b) (a < b ? a : b)
int video_encode_set_target_bps(int bps)
{
  if (NULL == g_ctx) {
    LOGE(TAG, "%s g_ctx null", g_codec_name);
    return -1;
  }

  bps = __MIN(bps, g_target_bps);
  if (g_current_bps == bps) {
    return 0;
  }

  if (0 > nvmpi_encoder_setBitrate(g_ctx, g_current_bps)) {
    LOGE(TAG, "%s nvmpi_encoder_setBitrate error.", g_codec_name);
    return -1;
  }

  g_current_bps = bps;
  LOGT(TAG, "%s nvmpi_encoder_setBitrate[%dKbps] success", g_codec_name, g_current_bps / 1000);
  return 0;
}

void video_encode_fini(void)
{
  nvmpi_encoder_close(g_ctx);
  g_ctx = NULL;

  if (g_buffer) {
    free(g_buffer);
    g_buffer = NULL;
  }

  LOGT(TAG, "%s encode fini success.", g_codec_name);
}
