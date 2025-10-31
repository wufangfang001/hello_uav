#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "agora_log.h"
#include "agora_rtc_api.h"
#include "video_config.h"
#include "video_capture.h"
#include "h264_encode.h"


#define TAG     "[demo]"

#define BWE_MIN_BITRATE   VIDEO_ENCODE_TARGET_BPS / 4
#define BWE_MAX_BITRATE   VIDEO_ENCODE_TARGET_BPS * 2
#define BWE_START_BITRATE VIDEO_ENCODE_TARGET_BPS

typedef struct {
  char            appid[64];
  char            token[512];
  char            channel[64];
  connection_id_t conn_id;
  pthread_t       worker_pid;
  bool            b_connected_flag;
  bool            b_stop_flag;
} app_t;

static app_t g_app = { 0 };

static void __signal_handler(int sig)
{
  LOGT(TAG, "receive signal[%d]", sig);

  switch (sig) {
    case SIGQUIT:
    case SIGABRT:
    case SIGINT:
      g_app.b_stop_flag = true;
      break;
    default:
      LOGE(TAG, "no handler, sig %d", sig);
  }
}

static void __app_print_usage(int argc, char **argv)
{
  printf("Usage: %s -a \'appid\' -c \'channel name\' -t \'token\'\n", argv[0]);
  printf(" -h, --help                 : show help info\n");
  printf(" -a, --appid                : appId\n");
  printf(" -c, --channel              : channel name\n");
  printf(" -t, --token                : token, default NULL\n");
}

static int __app_parse_args(int argc, char **argv)
{
  const char *short_option = "ha:c:t:";
  const struct option long_option[] = { { "help",         0, NULL, 'h' },
                                        { "appid",        1, NULL, 'a' },
                                        { "channel",      1, NULL, 'c' },
                                        { "token",        1, NULL, 't' },
                                        { 0,              0, 0,     0  } };
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
        snprintf(g_app.appid, sizeof(g_app.appid), "%s", optarg);
        LOGT(TAG, "appId: %s", g_app.appid);
        break;
      case 'c':
        snprintf(g_app.channel, sizeof(g_app.channel), "%s", optarg);
        LOGT(TAG, "channel: %s", g_app.channel);
        break;
      case 't':
        snprintf(g_app.token, sizeof(g_app.token), "%s", optarg);
        LOGT(TAG, "token: %s", g_app.token);
        break;
      default:
        break;
    }
  }

  if (g_app.appid[0] == '\0' ||
      g_app.channel[0] == '\0') {
    return -1;
  }

  return 0;
}

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
  g_app.b_connected_flag = true;
  connection_info_t conn_info = { 0 };
  agora_rtc_get_connection_info(g_app.conn_id, &conn_info);
  LOGT(TAG, "[conn-%u] Join the channel %s successfully, uid %u elapsed %d ms", conn_id, conn_info.channel_name, uid, elapsed);
}

static void __on_reconnecting(connection_id_t conn_id)
{
  g_app.b_connected_flag = false;
  LOGW(TAG, "[conn-%u] connection timeout, reconnecting", conn_id);
}

static void __on_connection_lost(connection_id_t conn_id)
{
  g_app.b_connected_flag = false;
  LOGW(TAG, "[conn-%u] Lost connection from the channel", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
  g_app.b_connected_flag = true;
  LOGT(TAG, "[conn-%u] Rejoin the channel successfully, uid %u elapsed %d ms", conn_id, uid, elapsed_ms);
}

static void __on_user_joined(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
  LOGT(TAG, "[conn-%u] Remote user \"%u\" has joined the channel, elapsed %d ms", conn_id, uid, elapsed_ms);
}

static void __on_user_offline(connection_id_t conn_id, uint32_t uid, int reason)
{
  LOGT(TAG, "[conn-%u] Remote user \"%u\" has left the channel, reason %d", conn_id, uid, reason);
}

static void __on_user_joined_with_user_account(connection_id_t conn_id, const user_info_t *user, int elapsed_ms)
{
  LOGT(TAG, "[conn-%u] Remote user %s - %u has joined the channel, elapsed %d ms", conn_id, user->user_account, user->uid, elapsed_ms);
}

static void __on_user_offline_with_user_account(connection_id_t conn_id, const user_info_t *user, int reason)
{
  LOGT(TAG, "[conn-%u] Remote user %s - %u has left the channel, reason %d", conn_id, user->user_account, user->uid, reason);
}

static void __on_user_info_updated(connection_id_t conn_id, const user_info_t *user)
{
  LOGT(TAG, "[conn-%u] Remote user info updated: %s - %u", conn_id, user->user_account, user->uid);
}

static void __on_user_mute_audio(connection_id_t conn_id, uint32_t uid, bool muted)
{
  LOGT(TAG, "[conn-%u] audio: uid=%u muted=%d", conn_id, uid, muted);
}

static void __on_user_mute_video(connection_id_t conn_id, uint32_t uid, bool muted)
{
  LOGT(TAG, "[conn-%u] video: uid=%u muted=%d", conn_id, uid, muted);
}

static void __on_error(connection_id_t conn_id, int code, const char *msg)
{
  if (code == ERR_VIDEO_SEND_OVER_BANDWIDTH_LIMIT) {
    LOGE(TAG, "Not enough uplink bandwdith. Error msg \"%s\"", msg);
    return;
  }

  if (code == ERR_INVALID_APP_ID) {
    LOGE(TAG, "Invalid App ID. Please double check. Error msg \"%s\"", msg);
  } else if (code == ERR_INVALID_CHANNEL_NAME) {
    LOGE(TAG, "Invalid channel name for conn_id %u. Please double check. Error msg \"%s\"", conn_id, msg);
  } else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
    LOGE(TAG, "Invalid token. Please double check. Error msg \"%s\"", msg);
  } else if (code == ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY) {
    LOGE(TAG, "Dynamic token is enabled but is not provided. Error msg \"%s\"", msg);
  } else {
    LOGW(TAG, "Error %d is captured. Error msg \"%s\"", code, msg);
  }

  g_app.b_stop_flag = true;
}

static void __on_license_failed(connection_id_t conn_id, int reason)
{
  LOGE(TAG, "License verified failed, reason: %d", reason);
  g_app.b_stop_flag = true;
}

static void __on_target_bitrate_changed(connection_id_t conn_id, uint32_t target_bps)
{
  LOGT(TAG, "[conn-%u] Bandwidth change detected. Please adjust encoder bitrate to %u kbps", conn_id, target_bps / 1000);
  h264_encode_set_target_bps(target_bps);
}

static void __on_key_frame_gen_req(connection_id_t conn_id, uint32_t uid, video_stream_type_e stream_type)
{
  LOGT(TAG, "[conn-%u] Frame loss detected. Please notify the encoder to generate key frame immediately", conn_id);
  h264_encode_generate_key_frame();
}

static void __on_rtc_stats(connection_id_t conn_id, rtc_stats_t stats)
{

}

static void __event_handler_register(agora_rtc_event_handler_t *event_handler)
{
  memset(event_handler, 0, sizeof(*event_handler));
  event_handler->on_join_channel_success       = __on_join_channel_success;
  event_handler->on_reconnecting               = __on_reconnecting,
  event_handler->on_connection_lost            = __on_connection_lost;
  event_handler->on_rejoin_channel_success     = __on_rejoin_channel_success;
  event_handler->on_user_joined                = __on_user_joined;
  event_handler->on_user_offline               = __on_user_offline;
  event_handler->on_user_mute_audio            = __on_user_mute_audio;
  event_handler->on_user_mute_video            = __on_user_mute_video;
  event_handler->on_target_bitrate_changed     = __on_target_bitrate_changed;
  event_handler->on_key_frame_gen_req          = __on_key_frame_gen_req;
  event_handler->on_error                      = __on_error;
  event_handler->on_license_validation_failure = __on_license_failed;
  event_handler->on_rtc_stats                  = __on_rtc_stats;
}

static void __rtc_service_option_init(rtc_service_option_t *service_opt)
{
  memset(service_opt, 0, sizeof(*service_opt));
  service_opt->area_code                       = AREA_CODE_GLOB;
  service_opt->log_cfg.log_path                = "./log";
  service_opt->log_cfg.log_disable             = false;
  service_opt->log_cfg.log_disable_desensitize = true;
  service_opt->log_cfg.log_level               = RTC_LOG_NOTICE;
  service_opt->use_string_uid                  = false;
  service_opt->domain_limit                    = false;
}

static void __channel_option_init(rtc_channel_options_t *channel_options)
{
  memset(channel_options, 0, sizeof(*channel_options));
  channel_options->auto_subscribe_audio = false;
  channel_options->auto_subscribe_video = false;
  channel_options->enable_audio_mixer = false;
  channel_options->audio_codec_opt.audio_codec_type = AUDIO_CODEC_DISABLED;
}

static void* __worker(void *args)
{
  uint8_t *yuv_data;
  int yuv_data_len;
  uint8_t *h264_data;
  int h264_data_len;
  uint8_t *yuv420;
  video_frame_info_t frame_info = {.data_type = VIDEO_DATA_TYPE_H264,
                                   .stream_type = VIDEO_STREAM_HIGH,
                                   .frame_type = VIDEO_FRAME_AUTO_DETECT,
                                   .frame_rate = 0,
                                   .rotation = VIDEO_ORIENTATION_0};

  h264_encode_init();
  video_capture_init();
  video_capture_start();

  yuv420 = (uint8_t *)malloc(CAPTURE_WIDTH * CAPTURE_HEIGHT / 2 * 3);

  while (!g_app.b_stop_flag) {
    yuv_data_len = video_capture_try_get_one_frame(&yuv_data);
    if (yuv_data_len > 0) {
      yuyv2yuv420(yuv_data, yuv420, CAPTURE_WIDTH, CAPTURE_HEIGHT);

      if (0 < (h264_data_len = h264_encode_encoding(yuv420, &h264_data))) {
        agora_rtc_send_video_data(g_app.conn_id, h264_data, h264_data_len, &frame_info);
      }

      video_capture_clear_one_frame();
    }
  }

  video_capture_stop();
  video_capture_fini();
  h264_encode_fini();
  free(yuv420);

  return NULL;
}

static void __worker_thread_create()
{
  pthread_create(&g_app.worker_pid, NULL, __worker, NULL);
}

static void __worker_thread_destroy()
{
  pthread_join(g_app.worker_pid, NULL);
}

int main(int argc, char **argv)
{
  int rval;
  agora_rtc_event_handler_t event_handler = { 0 };
  rtc_service_option_t service_opt = { 0 };
  rtc_channel_options_t channel_options = { 0 };

  if (__app_parse_args(argc, argv) < 0) {
    __app_print_usage(argc, argv);
    return 0;
  }

  signal(SIGINT, __signal_handler);
  __event_handler_register(&event_handler);
  __rtc_service_option_init(&service_opt);
  if (0 > (rval = agora_rtc_init(g_app.appid, &event_handler, &service_opt))) {
    LOGE(TAG, "Failed to initialize Agora sdk, reason: %s", agora_rtc_err_2_str(rval));
    return -1;
  }

  if (0 > (rval = agora_rtc_create_connection(&g_app.conn_id))) {
    LOGE(TAG, "Failed to create connection, reason: %s", agora_rtc_err_2_str(rval));
    return -1;
  }

  if (0 > (rval = agora_rtc_set_bwe_param(g_app.conn_id, BWE_MIN_BITRATE, BWE_MAX_BITRATE, BWE_START_BITRATE))) {
    LOGE(TAG, "Failed set bwe param, reason: %s", agora_rtc_err_2_str(rval));
    return -1;
  }

  if (0 > (rval = agora_rtc_join_channel(g_app.conn_id, g_app.channel, 0, g_app.token, &channel_options))) {
    LOGE(TAG, "Failed to join channel \"%s\", reason: %s", g_app.channel, agora_rtc_err_2_str(rval));
    return -1;
  }

  while (!g_app.b_connected_flag && !g_app.b_stop_flag) {
    usleep(100 * 1000);
  }

  __worker_thread_create();

  while (!g_app.b_stop_flag) {
    usleep(100 * 1000);
  }

  __worker_thread_destroy();
  agora_rtc_leave_channel(g_app.conn_id);
  agora_rtc_destroy_connection(g_app.conn_id);
  agora_rtc_fini();

  return 0;
}
