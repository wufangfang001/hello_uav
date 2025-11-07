#ifndef INC_AGORA_NATIVE_SDK_H_
#define INC_AGORA_NATIVE_SDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "agora_config.h"

int agora_native_init(agora_config_t *config);
void agora_native_fini(void);
int agora_native_send_h264_data(uint8_t *data, size_t len, bool isKeyFrame);
void agora_native_target_bitrate_change(uint32_t target_bps);
void agora_native_keyframe_request(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INC_AGORA_NATIVE_SDK_H_ */