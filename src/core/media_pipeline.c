#include <string.h>

#include "agora_log.h"
#include "media_backend.h"
#include "media_pipeline.h"

#define TAG "[media]"

static const media_backend_ops_t *g_backend_ops = NULL;
static bool g_initialized = false;
static bool g_started = false;

static const media_backend_ops_t *__media_pipeline_resolve_backend(void)
{
  if (g_backend_ops != NULL) {
    return g_backend_ops;
  }

  g_backend_ops = media_backend_get_ops();
  return g_backend_ops;
}

int media_pipeline_init(const media_video_config_t *config)
{
  const media_backend_ops_t *backend_ops;

  if (config == NULL) {
    LOGE(TAG, "media pipeline init failed: config is null");
    return -1;
  }

  if (g_initialized) {
    return 0;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops == NULL || backend_ops->init == NULL) {
    LOGE(TAG, "media pipeline init failed: backend ops unavailable");
    return -1;
  }

  if (backend_ops->init(config) != 0) {
    LOGE(TAG, "media pipeline init failed, backend=%s", media_pipeline_backend_name());
    return -1;
  }

  g_initialized = true;
  LOGT(TAG, "media pipeline init success, backend=%s", media_pipeline_backend_name());
  return 0;
}

int media_pipeline_start(void)
{
  const media_backend_ops_t *backend_ops;

  if (!g_initialized) {
    LOGE(TAG, "media pipeline start failed: not initialized");
    return -1;
  }

  if (g_started) {
    return 0;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops == NULL || backend_ops->start == NULL) {
    LOGE(TAG, "media pipeline start failed: backend start unavailable");
    return -1;
  }

  if (backend_ops->start() != 0) {
    LOGE(TAG, "media pipeline start failed, backend=%s", media_pipeline_backend_name());
    return -1;
  }

  g_started = true;
  return 0;
}

int media_pipeline_acquire_frame(media_encoded_frame_t *frame)
{
  const media_backend_ops_t *backend_ops;

  if (frame == NULL) {
    LOGE(TAG, "media pipeline acquire failed: frame is null");
    return -1;
  }

  memset(frame, 0, sizeof(*frame));

  if (!g_started) {
    LOGE(TAG, "media pipeline acquire failed: not started");
    return -1;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops == NULL || backend_ops->acquire_frame == NULL) {
    LOGE(TAG, "media pipeline acquire failed: backend acquire unavailable");
    return -1;
  }

  return backend_ops->acquire_frame(frame);
}

int media_pipeline_release_frame(media_encoded_frame_t *frame)
{
  const media_backend_ops_t *backend_ops;
  int ret;

  if (frame == NULL) {
    LOGE(TAG, "media pipeline release failed: frame is null");
    return -1;
  }

  if (!g_started) {
    memset(frame, 0, sizeof(*frame));
    return 0;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops == NULL || backend_ops->release_frame == NULL) {
    LOGE(TAG, "media pipeline release failed: backend release unavailable");
    return -1;
  }

  ret = backend_ops->release_frame(frame);
  if (ret == 0) {
    memset(frame, 0, sizeof(*frame));
  }

  return ret;
}

int media_pipeline_request_key_frame(void)
{
  const media_backend_ops_t *backend_ops;

  if (!g_initialized) {
    LOGE(TAG, "key frame request ignored: pipeline not initialized");
    return -1;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops == NULL || backend_ops->request_key_frame == NULL) {
    LOGE(TAG, "key frame request failed: backend request unavailable");
    return -1;
  }

  return backend_ops->request_key_frame();
}

int media_pipeline_set_target_bps(uint32_t target_bps)
{
  const media_backend_ops_t *backend_ops;

  if (!g_initialized) {
    LOGE(TAG, "bitrate update ignored: pipeline not initialized");
    return -1;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops == NULL || backend_ops->set_target_bps == NULL) {
    LOGE(TAG, "bitrate update failed: backend bitrate control unavailable");
    return -1;
  }

  return backend_ops->set_target_bps(target_bps);
}

int media_pipeline_stop(void)
{
  const media_backend_ops_t *backend_ops;
  int ret = 0;

  if (!g_started) {
    return 0;
  }

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops != NULL && backend_ops->stop != NULL) {
    ret = backend_ops->stop();
  }

  g_started = false;
  return ret;
}

void media_pipeline_fini(void)
{
  const media_backend_ops_t *backend_ops;

  if (!g_initialized) {
    return;
  }

  (void)media_pipeline_stop();

  backend_ops = __media_pipeline_resolve_backend();
  if (backend_ops != NULL && backend_ops->fini != NULL) {
    backend_ops->fini();
  }

  g_initialized = false;
}

const char *media_pipeline_backend_name(void)
{
  const media_backend_ops_t *backend_ops = __media_pipeline_resolve_backend();

  if (backend_ops == NULL || backend_ops->name == NULL) {
    return "unknown";
  }

  return backend_ops->name;
}
