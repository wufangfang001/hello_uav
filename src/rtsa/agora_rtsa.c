
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "agora_log.h"
#include "agora_rtc_api.h"
#include "agora_rtsa_sdk.h"


#define TAG     "[demo]"

static connection_id_t g_conn_id = CONNECTION_ID_INVALID;

static bool g_b_connected_flag = false;
static bool g_b_stop_flag = false;

static f_sdk_target_bitrate_change g_fun_bps_change = NULL;
static f_sdk_key_frame_request     g_fun_keyframe_request = NULL;
static f_sdk_should_stop           g_fun_stop = NULL;

static void __stop_flag_set(void)
{
  g_b_stop_flag = true;
  if (g_fun_stop) {
    g_fun_stop();
  }
}

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
  g_b_connected_flag = true;
  connection_info_t conn_info = { 0 };
  agora_rtc_get_connection_info(g_conn_id, &conn_info);
  LOGT(TAG, "[conn-%u] Join the channel %s successfully, uid %u elapsed %d ms", conn_id, conn_info.channel_name, uid, elapsed);
}

static void __on_reconnecting(connection_id_t conn_id)
{
  g_b_connected_flag = false;
  LOGW(TAG, "[conn-%u] connection timeout, reconnecting", conn_id);
}

static void __on_connection_lost(connection_id_t conn_id)
{
  g_b_connected_flag = false;
  LOGW(TAG, "[conn-%u] Lost connection from the channel", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
  g_b_connected_flag = true;
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

  __stop_flag_set();
}

static void __on_license_failed(connection_id_t conn_id, int reason)
{
  LOGE(TAG, "License verified failed, reason: %d", reason);
  __stop_flag_set();
}

static void __on_target_bitrate_changed(connection_id_t conn_id, uint32_t target_bps)
{
  LOGT(TAG, "[conn-%u] Bandwidth change detected. Please adjust encoder bitrate to %u kbps", conn_id, target_bps / 1000);
  if (g_fun_bps_change) {
    g_fun_bps_change(target_bps);
  }
}

static void __on_key_frame_gen_req(connection_id_t conn_id, uint32_t uid, video_stream_type_e stream_type)
{
  LOGT(TAG, "[conn-%u] Frame loss detected. Please notify the encoder to generate key frame immediately", conn_id);
  if (g_fun_keyframe_request) {
    g_fun_keyframe_request();
  }
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
  service_opt->log_cfg.log_path                = "./log/rtsa";
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
  channel_options->enable_audio_mixer   = false;
  channel_options->enable_lan_accelerate = false;
  channel_options->audio_codec_opt.audio_codec_type = AUDIO_CODEC_DISABLED;
}

static void __notify_function_register(agora_config_t *config)
{
  g_fun_bps_change       = config->f_bps_change;
  g_fun_keyframe_request = config->f_key_frame;
  g_fun_stop             = config->f_stop;
}

static char *__get_token(char *app_id, char *token)
{
  if (NULL == token || token[0] == '\0' || 0 == strcmp(app_id, token)) {
    return NULL;
  }

  return token;
}

int agora_rtsa_init(agora_config_t *config)
{
  int rval;
  agora_rtc_event_handler_t event_handler = { 0 };
  rtc_service_option_t service_opt = { 0 };
  rtc_channel_options_t channel_options = { 0 };

  __notify_function_register(config);
  __event_handler_register(&event_handler);
  __rtc_service_option_init(&service_opt);

  LOGT(TAG, "welcome to rtsa. version=%s", agora_rtc_get_version());

  if (0 > (rval = agora_rtc_init(config->appid, &event_handler, &service_opt))) {
    LOGE(TAG, "Failed to initialize Agora sdk, reason: %s", agora_rtc_err_2_str(rval));
    goto L_AGORA_RTC_INIT_FAILED;
  }

  if (0 > (rval = agora_rtc_create_connection(&g_conn_id))) {
    LOGE(TAG, "Failed to create connection, reason: %s", agora_rtc_err_2_str(rval));
    goto L_AGORA_RTC_CREATE_CONNECTION_FAILED;
  }

  if (0 > (rval = agora_rtc_set_bwe_param(g_conn_id, config->video_bps / 4, config->video_bps, config->video_bps/2))) {
    LOGE(TAG, "Failed set bwe param, reason: %s", agora_rtc_err_2_str(rval));
    goto L_AGORA_RTC_SET_BWE_PARAM_FAILED;
  }

  channel_options.enable_lan_accelerate = config->b_enable_lan_accelerate;

  if (0 > (rval = agora_rtc_join_channel(g_conn_id, config->channel, 0, __get_token(config->appid, config->token), &channel_options))) {
    LOGE(TAG, "Failed to join channel \"%s\", reason: %s", config->channel, agora_rtc_err_2_str(rval));
    goto L_AGORA_RTC_JOIN_CHANNEL_FAILED;
  }

  while (!g_b_connected_flag && !g_b_stop_flag) {
    usleep(100 * 1000);
  }

  LOGT(TAG, "agora rtsa init success.");
  return 0;

L_AGORA_RTC_JOIN_CHANNEL_FAILED:
L_AGORA_RTC_SET_BWE_PARAM_FAILED:
  agora_rtc_destroy_connection(g_conn_id);

L_AGORA_RTC_CREATE_CONNECTION_FAILED:
  agora_rtc_fini();

L_AGORA_RTC_INIT_FAILED:

  return (0 == rval ? 0 : -1);
}

void agora_rtsa_fini(void)
{
  agora_rtc_leave_channel(g_conn_id);
  agora_rtc_destroy_connection(g_conn_id);
  agora_rtc_fini();
  LOGT(TAG, "agora rtsa fini success.");
}

int agora_rtsa_send_video_data(uint8_t *data, size_t len, VideoCodecType codec_type)
{
  video_frame_info_t frame_info = {.data_type = (codec_type == VideoCodecTypeH265) ? VIDEO_DATA_TYPE_H265 : VIDEO_DATA_TYPE_H264,
                                   .stream_type = VIDEO_STREAM_HIGH,
                                   .frame_type = VIDEO_FRAME_AUTO_DETECT,
                                   .frame_rate = 0,
                                   .rotation = VIDEO_ORIENTATION_0};
  return agora_rtc_send_video_data(g_conn_id, data, len, &frame_info);
}
