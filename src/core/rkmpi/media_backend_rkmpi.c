#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "agora_log.h"
#include "media_backend.h"
#include "rk_common.h"
#include "rk_comm_mb.h"
#include "rk_comm_rc.h"
#include "rk_comm_sys.h"
#include "rk_comm_venc.h"
#include "rk_comm_vi.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_sys.h"
#include "rk_mpi_venc.h"
#include "rk_mpi_vi.h"

#define TAG "[media-rk]"
#define RKMPI_VENC_CHN 0

typedef struct {
  VENC_PACK_S *pack;
} rkmpi_frame_priv_t;

static media_video_config_t g_config = { 0 };
static VI_DEV_ATTR_S g_vi_dev_attr = { 0 };
static VI_DEV_BIND_PIPE_S g_vi_bind_pipe = { 0 };
static VI_CHN_ATTR_S g_vi_chn_attr = { 0 };
static VENC_CHN_ATTR_S g_venc_chn_attr = { 0 };
static VENC_STREAM_S g_venc_stream = { 0 };
static MPP_CHN_S g_vi_src_chn = { 0 };
static MPP_CHN_S g_venc_dst_chn = { 0 };
static rkmpi_frame_priv_t g_frame_priv = { 0 };
static uint8_t *g_stream_buffer = NULL;
static size_t g_stream_buffer_len = 0;
static size_t g_stream_payload_len = 0;
static uint32_t g_current_bps = 0;
static bool g_sys_initialized = false;
static bool g_vi_initialized = false;
static bool g_venc_initialized = false;
static bool g_bound = false;
static bool g_started = false;
static bool g_frame_held = false;

static void __rkmpi_reset_channels(void)
{
  memset(&g_vi_src_chn, 0, sizeof(g_vi_src_chn));
  memset(&g_venc_dst_chn, 0, sizeof(g_venc_dst_chn));

  g_vi_src_chn.enModId = RK_ID_VI;
  g_vi_src_chn.s32DevId = (RK_S32)g_config.device_id;
  g_vi_src_chn.s32ChnId = (RK_S32)g_config.channel_id;

  g_venc_dst_chn.enModId = RK_ID_VENC;
  g_venc_dst_chn.s32DevId = RKMPI_VENC_CHN;
  g_venc_dst_chn.s32ChnId = RKMPI_VENC_CHN;
}

static bool __rkmpi_pack_is_key_frame(const VENC_PACK_S *pack)
{
  if (pack == NULL) {
    return false;
  }

  if (g_config.codec_type == VideoCodecTypeH265) {
    return pack->DataType.enH265EType == H265E_NALU_IDRSLICE ||
           pack->DataType.enH265EType == H265E_NALU_ISLICE;
  }

  return pack->DataType.enH264EType == H264E_NALU_IDRSLICE ||
         pack->DataType.enH264EType == H264E_NALU_ISLICE;
}

static size_t __rkmpi_pack_copy_payload(const VENC_PACK_S *pack, uint8_t *dst, size_t dst_len)
{
  uint8_t *src;
  size_t valid_len;

  if (pack == NULL || dst == NULL) {
    return 0;
  }

  if (pack->u32Offset >= pack->u32Len) {
    return 0;
  }

  src = (uint8_t *)RK_MPI_MB_Handle2VirAddr(pack->pMbBlk);
  if (src == NULL) {
    return 0;
  }

  valid_len = (size_t)(pack->u32Len - pack->u32Offset);
  if (valid_len > dst_len) {
    return 0;
  }

  memcpy(dst, src + pack->u32Offset, valid_len);
  return valid_len;
}

static int __rkmpi_ensure_stream_buffer(size_t required_len)
{
  uint8_t *new_buffer;
  size_t new_len = g_stream_buffer_len == 0 ? 256 * 1024 : g_stream_buffer_len;

  while (new_len < required_len) {
    new_len *= 2;
  }

  if (new_len == g_stream_buffer_len) {
    return 0;
  }

  new_buffer = (uint8_t *)realloc(g_stream_buffer, new_len);
  if (new_buffer == NULL) {
    LOGE(TAG, "realloc stream buffer failed, len=%zu", new_len);
    return -1;
  }

  g_stream_buffer = new_buffer;
  g_stream_buffer_len = new_len;
  return 0;
}

static bool __rkmpi_use_virtual_pipe(const media_video_config_t *config)
{
  if (config == NULL || config->device_name == NULL || config->device_name[0] == '\0') {
    return false;
  }

  if (config->device_name[0] == '/') {
    return false;
  }

  return config->pipe_id == 0 || config->pipe_id == VI_VIR_PIPE_ID || config->device_id == VI_VIR_PIPE_ID;
}

static void __rkmpi_log_pipe_selection(void)
{
  if (g_config.pipe_id == VI_VIR_PIPE_ID) {
    LOGT(TAG, "VI using virtual pipe id=%d, entity=%s", g_config.pipe_id, g_config.device_name ? g_config.device_name : "<null>");
  } else {
    LOGT(TAG, "VI using physical pipe id=%d, device_id=%d, channel_id=%d", g_config.pipe_id, g_config.device_id, g_config.channel_id);
  }
}

static void __rkmpi_fill_vi_defaults(const media_video_config_t *config)
{
  memset(&g_vi_dev_attr, 0, sizeof(g_vi_dev_attr));
  g_vi_dev_attr.enBufType = VI_V4L2_MEMORY_TYPE_DMABUF;
  g_vi_dev_attr.u32BufCount = 3;
  g_vi_dev_attr.stMaxSize.u32Width = g_config.width;
  g_vi_dev_attr.stMaxSize.u32Height = g_config.height;

  memset(&g_vi_bind_pipe, 0, sizeof(g_vi_bind_pipe));
  g_vi_bind_pipe.u32Num = 1;
  g_vi_bind_pipe.PipeId[0] = (VI_PIPE)g_config.pipe_id;

  memset(&g_vi_chn_attr, 0, sizeof(g_vi_chn_attr));
  g_vi_chn_attr.stSize.u32Width = g_config.width;
  g_vi_chn_attr.stSize.u32Height = g_config.height;
  g_vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
  g_vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE;
  g_vi_chn_attr.u32Depth = 1;
  g_vi_chn_attr.stFrameRate.s32SrcFrameRate = -1;
  g_vi_chn_attr.stFrameRate.s32DstFrameRate = -1;
  g_vi_chn_attr.stIspOpt.u32BufCount = 3;
  g_vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
  g_vi_chn_attr.stIspOpt.enCaptureType = VI_V4L2_CAPTURE_TYPE_VIDEO_CAPTURE;

  if (g_config.pipe_id == VI_VIR_PIPE_ID && g_config.device_name != NULL && g_config.device_name[0] != '\0') {
    snprintf(g_vi_chn_attr.stIspOpt.aEntityName,
             sizeof(g_vi_chn_attr.stIspOpt.aEntityName),
             "%s",
             g_config.device_name);
  }
}

static void __rkmpi_fill_venc_defaults(const media_video_config_t *config)
{
  RK_CODEC_ID_E codec_id = config->codec_type == VideoCodecTypeH265 ? RK_VIDEO_ID_HEVC : RK_VIDEO_ID_AVC;

  memset(&g_venc_chn_attr, 0, sizeof(g_venc_chn_attr));
  g_venc_chn_attr.stVencAttr.enType = codec_id;
  g_venc_chn_attr.stVencAttr.enPixelFormat = g_vi_chn_attr.enPixelFormat;
  g_venc_chn_attr.stVencAttr.u32PicWidth = config->width;
  g_venc_chn_attr.stVencAttr.u32PicHeight = config->height;
  g_venc_chn_attr.stVencAttr.u32VirWidth = config->width;
  g_venc_chn_attr.stVencAttr.u32VirHeight = config->height;
  g_venc_chn_attr.stVencAttr.u32StreamBufCnt = 5;
  g_venc_chn_attr.stVencAttr.u32BufSize = config->width * config->height * 3 / 2;
  g_venc_chn_attr.stVencAttr.bByFrame = RK_TRUE;

  if (config->codec_type == VideoCodecTypeH265) {
    g_venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
    g_venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = config->fps * 2;
    g_venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = config->fps;
    g_venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
    g_venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = config->fps;
    g_venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
    g_venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = config->bps / 1000;
    g_venc_chn_attr.stRcAttr.stH265Cbr.u32StatTime = 1;
  } else {
    g_venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    g_venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = config->fps * 2;
    g_venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = config->fps;
    g_venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    g_venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = config->fps;
    g_venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    g_venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = config->bps / 1000;
    g_venc_chn_attr.stRcAttr.stH264Cbr.u32StatTime = 1;
  }
}

static int __rkmpi_vi_init(void)
{
  RK_S32 ret;

  ret = RK_MPI_VI_GetDevAttr((VI_DEV)g_config.device_id, &g_vi_dev_attr);
  if (ret == RK_ERR_VI_NOT_CONFIG) {
    ret = RK_MPI_VI_SetDevAttr((VI_DEV)g_config.device_id, &g_vi_dev_attr);
    if (ret != RK_SUCCESS) {
      LOGE(TAG, "RK_MPI_VI_SetDevAttr failed, ret=0x%x", ret);
      return -1;
    }
  } else if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VI_GetDevAttr failed, ret=0x%x", ret);
    return -1;
  }

  ret = RK_MPI_VI_GetDevIsEnable((VI_DEV)g_config.device_id);
  if (ret != RK_SUCCESS) {
    ret = RK_MPI_VI_EnableDev((VI_DEV)g_config.device_id);
    if (ret != RK_SUCCESS) {
      LOGE(TAG, "RK_MPI_VI_EnableDev failed, ret=0x%x", ret);
      return -1;
    }

    ret = RK_MPI_VI_SetDevBindPipe((VI_DEV)g_config.device_id, &g_vi_bind_pipe);
    if (ret != RK_SUCCESS) {
      LOGE(TAG, "RK_MPI_VI_SetDevBindPipe failed, ret=0x%x", ret);
      RK_MPI_VI_DisableDev((VI_DEV)g_config.device_id);
      return -1;
    }
  }

  ret = RK_MPI_VI_SetChnAttr((VI_PIPE)g_config.pipe_id, (VI_CHN)g_config.channel_id, &g_vi_chn_attr);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VI_SetChnAttr failed, ret=0x%x", ret);
    RK_MPI_VI_DisableDev((VI_DEV)g_config.device_id);
    return -1;
  }

  ret = RK_MPI_VI_EnableChn((VI_PIPE)g_config.pipe_id, (VI_CHN)g_config.channel_id);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VI_EnableChn failed, ret=0x%x", ret);
    RK_MPI_VI_DisableDev((VI_DEV)g_config.device_id);
    return -1;
  }

  g_vi_initialized = true;
  return 0;
}

static int __rkmpi_venc_init(void)
{
  RK_S32 ret;
  VENC_RECV_PIC_PARAM_S recv_param = { .s32RecvPicNum = -1 };

  ret = RK_MPI_VENC_CreateChn(RKMPI_VENC_CHN, &g_venc_chn_attr);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VENC_CreateChn failed, ret=0x%x", ret);
    return -1;
  }

  ret = RK_MPI_VENC_StartRecvFrame(RKMPI_VENC_CHN, &recv_param);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VENC_StartRecvFrame failed, ret=0x%x", ret);
    RK_MPI_VENC_DestroyChn(RKMPI_VENC_CHN);
    return -1;
  }

  g_frame_priv.pack = (VENC_PACK_S *)calloc(1, sizeof(*g_frame_priv.pack));
  if (g_frame_priv.pack == NULL) {
    LOGE(TAG, "alloc venc pack failed");
    RK_MPI_VENC_StopRecvFrame(RKMPI_VENC_CHN);
    RK_MPI_VENC_DestroyChn(RKMPI_VENC_CHN);
    return -1;
  }

  memset(&g_venc_stream, 0, sizeof(g_venc_stream));
  g_venc_stream.pstPack = g_frame_priv.pack;
  g_venc_stream.u32PackCount = 1;
  g_venc_initialized = true;
  return 0;
}

static void __rkmpi_release_stream_internal(void)
{
  RK_S32 ret;

  if (!g_frame_held) {
    return;
  }

  ret = RK_MPI_VENC_ReleaseStream(RKMPI_VENC_CHN, &g_venc_stream);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VENC_ReleaseStream failed, ret=0x%x", ret);
  }

  memset(g_frame_priv.pack, 0, sizeof(*g_frame_priv.pack));
  g_venc_stream.u32PackCount = 1;
  g_stream_payload_len = 0;
  g_frame_held = false;
}

static int __rkmpi_bind(void)
{
  RK_S32 ret;

  ret = RK_MPI_SYS_Bind(&g_vi_src_chn, &g_venc_dst_chn);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_SYS_Bind failed, ret=0x%x", ret);
    return -1;
  }

  g_bound = true;
  return 0;
}

static void __rkmpi_unbind(void)
{
  RK_S32 ret;

  if (!g_bound) {
    return;
  }

  ret = RK_MPI_SYS_UnBind(&g_vi_src_chn, &g_venc_dst_chn);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_SYS_UnBind failed, ret=0x%x", ret);
  }

  g_bound = false;
}

static int __rkmpi_update_bitrate(uint32_t target_bps)
{
  RK_S32 ret;
  VENC_CHN_ATTR_S chn_attr;
  uint32_t target_kbps = target_bps / 1000;

  memset(&chn_attr, 0, sizeof(chn_attr));
  ret = RK_MPI_VENC_GetChnAttr(RKMPI_VENC_CHN, &chn_attr);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VENC_GetChnAttr failed, ret=0x%x", ret);
    return -1;
  }

  if (g_config.codec_type == VideoCodecTypeH265) {
    chn_attr.stRcAttr.stH265Cbr.u32BitRate = target_kbps;
  } else {
    chn_attr.stRcAttr.stH264Cbr.u32BitRate = target_kbps;
  }

  ret = RK_MPI_VENC_SetChnAttr(RKMPI_VENC_CHN, &chn_attr);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VENC_SetChnAttr failed, ret=0x%x", ret);
    return -1;
  }

  g_venc_chn_attr = chn_attr;
  g_current_bps = target_bps;
  return 0;
}

static int __rkmpi_init(const media_video_config_t *config)
{
  RK_S32 ret;

  memset(&g_config, 0, sizeof(g_config));
  g_config = *config;

  if (__rkmpi_use_virtual_pipe(config)) {
    g_config.device_id = VI_VIR_PIPE_ID;
    g_config.pipe_id = VI_VIR_PIPE_ID;
  } else if (g_config.pipe_id == 0 && g_config.device_id != 0) {
    g_config.pipe_id = g_config.device_id;
  }

  __rkmpi_log_pipe_selection();
  __rkmpi_fill_vi_defaults(config);
  __rkmpi_fill_venc_defaults(config);
  __rkmpi_reset_channels();

  ret = RK_MPI_SYS_Init();
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_SYS_Init failed, ret=0x%x", ret);
    return -1;
  }
  g_sys_initialized = true;

  if (__rkmpi_vi_init() != 0) {
    goto fail;
  }

  if (__rkmpi_venc_init() != 0) {
    goto fail;
  }

  if (__rkmpi_bind() != 0) {
    goto fail;
  }

  if (__rkmpi_ensure_stream_buffer(config->width * config->height) != 0) {
    goto fail;
  }

  g_current_bps = config->bps;
  return 0;

fail:
  if (g_bound) {
    __rkmpi_unbind();
  }
  if (g_venc_initialized) {
    RK_MPI_VENC_StopRecvFrame(RKMPI_VENC_CHN);
    RK_MPI_VENC_DestroyChn(RKMPI_VENC_CHN);
    free(g_frame_priv.pack);
    g_frame_priv.pack = NULL;
    g_venc_initialized = false;
  }
  if (g_vi_initialized) {
    RK_MPI_VI_DisableChn((VI_PIPE)g_config.pipe_id, (VI_CHN)g_config.channel_id);
    RK_MPI_VI_DisableDev((VI_DEV)g_config.device_id);
    g_vi_initialized = false;
  }
  if (g_sys_initialized) {
    RK_MPI_SYS_Exit();
    g_sys_initialized = false;
  }
  free(g_stream_buffer);
  g_stream_buffer = NULL;
  g_stream_buffer_len = 0;
  g_stream_payload_len = 0;
  memset(&g_config, 0, sizeof(g_config));
  return -1;
}

static int __rkmpi_start(void)
{
  if (!g_bound || !g_venc_initialized) {
    LOGE(TAG, "start failed: rkmpi backend not initialized");
    return -1;
  }

  g_started = true;
  return 0;
}

static int __rkmpi_acquire_frame(media_encoded_frame_t *frame)
{
  RK_S32 ret;
  size_t total_len = 0;
  bool is_key_frame = false;
  uint32_t i;

  if (g_frame_held) {
    LOGE(TAG, "acquire failed: previous frame has not been released");
    return -1;
  }

  memset(g_frame_priv.pack, 0, sizeof(*g_frame_priv.pack));
  g_venc_stream.u32PackCount = 1;

  ret = RK_MPI_VENC_GetStream(RKMPI_VENC_CHN, &g_venc_stream, -1);
  if (ret != RK_SUCCESS) {
    return ret == RK_ERR_VENC_BUF_EMPTY ? 0 : -1;
  }

  if (g_venc_stream.u32PackCount == 0) {
    RK_MPI_VENC_ReleaseStream(RKMPI_VENC_CHN, &g_venc_stream);
    return 0;
  }

  for (i = 0; i < g_venc_stream.u32PackCount; ++i) {
    VENC_PACK_S *pack = &g_venc_stream.pstPack[i];
    size_t valid_len = pack->u32Len > pack->u32Offset ? (size_t)(pack->u32Len - pack->u32Offset) : 0;

    if (valid_len == 0) {
      continue;
    }

    if (__rkmpi_ensure_stream_buffer(total_len + valid_len) != 0) {
      __rkmpi_release_stream_internal();
      return -1;
    }

    if (__rkmpi_pack_copy_payload(pack, g_stream_buffer + total_len, g_stream_buffer_len - total_len) != valid_len) {
      __rkmpi_release_stream_internal();
      return -1;
    }

    total_len += valid_len;
    is_key_frame = is_key_frame || __rkmpi_pack_is_key_frame(pack);
  }

  if (total_len == 0) {
    __rkmpi_release_stream_internal();
    return 0;
  }

  g_stream_payload_len = total_len;
  g_frame_held = true;

  frame->data = g_stream_buffer;
  frame->len = total_len;
  frame->is_key_frame = is_key_frame;
  frame->priv = &g_frame_priv;
  return 1;
}

static int __rkmpi_release_frame(media_encoded_frame_t *frame)
{
  (void)frame;
  __rkmpi_release_stream_internal();
  return 0;
}

static int __rkmpi_request_key_frame(void)
{
  RK_S32 ret;

  ret = RK_MPI_VENC_RequestIDR(RKMPI_VENC_CHN, RK_FALSE);
  if (ret != RK_SUCCESS) {
    LOGE(TAG, "RK_MPI_VENC_RequestIDR failed, ret=0x%x", ret);
    return -1;
  }

  return 0;
}

static int __rkmpi_set_target_bps(uint32_t target_bps)
{
  if (!g_venc_initialized) {
    LOGE(TAG, "set target bps failed: venc not initialized");
    return -1;
  }

  if (g_current_bps == target_bps) {
    return 0;
  }

  return __rkmpi_update_bitrate(target_bps);
}

static int __rkmpi_stop(void)
{
  if (!g_started) {
    return 0;
  }

  __rkmpi_release_stream_internal();
  g_started = false;
  return 0;
}

static void __rkmpi_fini(void)
{
  __rkmpi_stop();
  __rkmpi_unbind();

  if (g_venc_initialized) {
    RK_MPI_VENC_StopRecvFrame(RKMPI_VENC_CHN);
    RK_MPI_VENC_DestroyChn(RKMPI_VENC_CHN);
    g_venc_initialized = false;
  }

  if (g_vi_initialized) {
    RK_MPI_VI_DisableChn((VI_PIPE)g_config.pipe_id, (VI_CHN)g_config.channel_id);
    RK_MPI_VI_DisableDev((VI_DEV)g_config.device_id);
    g_vi_initialized = false;
  }

  if (g_sys_initialized) {
    RK_MPI_SYS_Exit();
    g_sys_initialized = false;
  }

  free(g_frame_priv.pack);
  g_frame_priv.pack = NULL;
  memset(&g_venc_stream, 0, sizeof(g_venc_stream));

  free(g_stream_buffer);
  g_stream_buffer = NULL;
  g_stream_buffer_len = 0;
  g_stream_payload_len = 0;
  g_current_bps = 0;
  g_frame_held = false;
  g_started = false;
  g_bound = false;
  memset(&g_config, 0, sizeof(g_config));
}

static const media_backend_ops_t g_rkmpi_backend_ops = {
  .name = "rkmpi",
  .init = __rkmpi_init,
  .start = __rkmpi_start,
  .acquire_frame = __rkmpi_acquire_frame,
  .release_frame = __rkmpi_release_frame,
  .request_key_frame = __rkmpi_request_key_frame,
  .set_target_bps = __rkmpi_set_target_bps,
  .stop = __rkmpi_stop,
  .fini = __rkmpi_fini,
};

const media_backend_ops_t *media_backend_get_ops(void)
{
  return &g_rkmpi_backend_ops;
}
