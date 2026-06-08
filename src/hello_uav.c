#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "agora_config.h"
#include "agora_log.h"
#include "media_pipeline.h"
#include "agora_rtsa_sdk.h"
#include "agora_native_sdk.h"

#define TAG "[demo]"

static void __sdk_target_bitrate_change(uint32_t target_bps);
static void __sdk_key_frame_request(void);
static void __sdk_should_stop(void);

static pthread_t      g_worker_pid;
static bool           g_b_stopped = false;
static agora_config_t g_config = { 0 };

static void __config_init(void)
{
  strncpy(g_config.appid, "aab8b8f5a8cd4469a63042fcfafe7063", sizeof(g_config.appid));
  memset(g_config.token, 0, sizeof(g_config.token));
  strncpy(g_config.channel, "hello-uav", sizeof(g_config.channel));
  memset(g_config.video_device_name, 0, sizeof(g_config.video_device_name));
  strncpy(g_config.video_device_name, "/dev/video0", sizeof(g_config.video_device_name));

  g_config.video_width  = 1280;
  g_config.video_height = 720;
  g_config.video_bps    = 1500000;
  g_config.video_fps    = 25;
  g_config.video_device_id = 0;
  g_config.video_pipe_id = 0;
  g_config.video_channel_id = 1;

  g_config.b_enable_multi_path = false;
  g_config.b_enable_rtsa       = false;

  g_config.f_bps_change = __sdk_target_bitrate_change;
  g_config.f_key_frame  = __sdk_key_frame_request;
  g_config.f_stop       = __sdk_should_stop;
}

static void __signal_handler(int sig)
{
  LOGT(TAG, "receive signal[%d]", sig);

  switch (sig) {
    case SIGQUIT:
    case SIGABRT:
    case SIGINT:
      g_b_stopped = true;
      break;
    default:
      LOGE(TAG, "no handler, sig %d", sig);
  }
}

static void __signal_register()
{
  signal(SIGINT, __signal_handler);
  signal(SIGQUIT, __signal_handler);
  signal(SIGABRT, __signal_handler);
}

static void __config_print()
{
  LOGT(TAG, "---------------------------------");
  LOGT(TAG, "-------config information--------");
  LOGT(TAG, "appId=%s", g_config.appid);
  LOGT(TAG, "token=%s", g_config.token);
  LOGT(TAG, "channel=%s", g_config.channel);
  LOGT(TAG, "bitrate=%u", g_config.video_bps);
  LOGT(TAG, "fps=%u", g_config.video_fps);
  LOGT(TAG, "videoDevice=%s", g_config.video_device_name);
  LOGT(TAG, "videoDevId=%u", g_config.video_device_id);
  LOGT(TAG, "videoPipeId=%u", g_config.video_pipe_id);
  LOGT(TAG, "videoChnId=%u", g_config.video_channel_id);
  LOGT(TAG, "enableMultiPath=%u", g_config.b_enable_multi_path);
  LOGT(TAG, "enableRtsaSdk=%u", g_config.b_enable_rtsa);
  LOGT(TAG, "codec=%s", g_config.video_codec_type == VideoCodecTypeH265 ? "h265" : "h264");
  LOGT(TAG, "---------------------------------");
}

static void __app_print_usage(int argc, char **argv)
{
  printf("Usage: %s -a \'appid\' -c \'channel name\' -t \'token\'\n", argv[0]);
  printf(" -h, --help                 : show help info\n");
  printf(" -a, --appId                : appId\n");
  printf(" -b, --bitrate              : target bitrate\n");
  printf(" -c, --channelId            : channel name\n");
  printf(" -d, --videoDevice          : video device/entity name, default /dev/video0\n");
  printf(" -D, --videoDevId           : media device id, default 0\n");
  printf(" -f, --fps                  : video fps\n");
  printf(" -m, --enableMultipath      : enable multipath\n");
  printf(" -p, --videoPipeId          : media pipe id, default 0\n");
  printf(" -r, --enableRtsaSdk        : use rtsa sdk, otherwise use native sdk\n");
  printf(" -t, --token                : token, default NULL\n");
  printf(" -v, --videoChnId           : media channel id, default 1\n");
  printf(" -C, --codec                : video codec type (h264 or h265), default h264\n");
}

static int __app_parse_args(int argc, char **argv)
{
  const char *short_option = "ha:b:c:d:D:f:m:p:r:t:v:C:";
  const struct option long_option[] = { { "help",            0, NULL, 'h' },
                                        { "appId",           1, NULL, 'a' },
                                        { "bitrate",         1, NULL, 'b' },
                                        { "channelId",       1, NULL, 'c' },
                                        { "videoDevice",     1, NULL, 'd' },
                                        { "videoDevId",      1, NULL, 'D' },
                                        { "fps",             1, NULL, 'f' },
                                        { "enableMultipath", 1, NULL, 'm' },
                                        { "videoPipeId",     1, NULL, 'p' },
                                        { "enableRtsaSdk",   1, NULL, 'r' },
                                        { "token",           1, NULL, 't' },
                                        { "videoChnId",      1, NULL, 'v' },
                                        { "codec",           1, NULL, 'C' },
                                        { 0,                 0, 0,     0  } };
  int ch = -1;
  int optidx = 0;

  while (1) {
    ch = getopt_long(argc, argv, short_option, long_option, &optidx);
    if (ch == -1) {
      break;
    }

    switch (ch) {
      case 'h':
        return -1;
      case 'a':
        snprintf(g_config.appid, sizeof(g_config.appid), "%s", optarg);
        break;
      case 'b':
        g_config.video_bps = strtol(optarg, NULL, 10);
        break;
      case 'c':
        snprintf(g_config.channel, sizeof(g_config.channel), "%s", optarg);
        break;
      case 'd':
        snprintf(g_config.video_device_name, sizeof(g_config.video_device_name), "%s", optarg);
        break;
      case 'D':
        g_config.video_device_id = strtol(optarg, NULL, 10);
        break;
      case 'f':
        g_config.video_fps = strtol(optarg, NULL, 10);
        break;
      case 'm':
        g_config.b_enable_multi_path = strtol(optarg, NULL, 10);
        break;
      case 'p':
        g_config.video_pipe_id = strtol(optarg, NULL, 10);
        break;
      case 'r':
        g_config.b_enable_rtsa = strtol(optarg, NULL, 10);
        break;
      case 't':
        snprintf(g_config.token, sizeof(g_config.token), "%s", optarg);
        break;
      case 'v':
        g_config.video_channel_id = strtol(optarg, NULL, 10);
        break;
      case 'C':
        if (0 == strcmp(optarg, "h265")) {
          g_config.video_codec_type = VideoCodecTypeH265;
        } else {
          g_config.video_codec_type = VideoCodecTypeH264;
        }
        break;
      default:
        break;
    }
  }

  return 0;
}

static void __sdk_target_bitrate_change(uint32_t target_bps)
{
  LOGT(TAG, "target bps change. bps=%u", target_bps);
  media_pipeline_set_target_bps(target_bps);
}

static void __sdk_key_frame_request(void)
{
  LOGT(TAG, "key frame request.");
  media_pipeline_request_key_frame();
}

static void __sdk_should_stop(void)
{
  LOGT(TAG, "sdk stop.");
  g_b_stopped = true;
}

static int __agora_rtc_init(void)
{
  if (g_config.b_enable_rtsa) {
    return agora_rtsa_init(&g_config);
  }

  return agora_native_init(&g_config);
}

static void __agora_rtc_fini()
{
  if (g_config.b_enable_rtsa) {
    agora_rtsa_fini();
  } else {
    agora_native_fini();
  }
}

static void __agora_rtc_send_video(uint8_t *data, size_t len, bool keyframe)
{
  if (g_config.b_enable_rtsa) {
    agora_rtsa_send_video_data(data, len, g_config.video_codec_type);
  } else {
    agora_native_send_video_data(data, len, keyframe, g_config.video_codec_type);
  }
}

static void* __worker(void *args)
{
  media_video_config_t video_config = {
    .codec_type = g_config.video_codec_type,
    .device_name = g_config.video_device_name,
    .width = g_config.video_width,
    .height = g_config.video_height,
    .fps = g_config.video_fps,
    .bps = g_config.video_bps,
    .device_id = g_config.video_device_id,
    .pipe_id = g_config.video_pipe_id,
    .channel_id = g_config.video_channel_id,
  };
  media_encoded_frame_t frame = { 0 };
  int ret;

  (void)args;

  if (__agora_rtc_init() != 0) {
    LOGE(TAG, "agora rtc init failed");
    return NULL;
  }

  if (media_pipeline_init(&video_config) != 0) {
    LOGE(TAG, "media pipeline init failed");
    goto exit_sdk;
  }

  if (media_pipeline_start() != 0) {
    LOGE(TAG, "media pipeline start failed");
    goto exit_media;
  }

  LOGT(TAG, "media backend=%s", media_pipeline_backend_name());

  while (!g_b_stopped) {
    ret = media_pipeline_acquire_frame(&frame);
    if (ret < 0) {
      LOGE(TAG, "media pipeline acquire frame failed");
      break;
    }

    if (ret == 0) {
      continue;
    }

    __agora_rtc_send_video(frame.data, frame.len, frame.is_key_frame);
    media_pipeline_release_frame(&frame);
  }

  media_pipeline_release_frame(&frame);
  media_pipeline_stop();

exit_media:
  media_pipeline_fini();

exit_sdk:
  __agora_rtc_fini();

  return NULL;
}

int main(int argc, char **argv)
{
  __config_init();

  if (__app_parse_args(argc, argv) < 0) {
    __app_print_usage(argc, argv);
    return 0;
  }

  __config_print();
  __signal_register();
  pthread_create(&g_worker_pid, NULL, __worker, NULL);
  pthread_join(g_worker_pid, NULL);
  return 0;
}
