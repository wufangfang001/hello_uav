#ifndef INC_AGORA_RTSA_SDK_H_
#define INC_AGORA_RTSA_SDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "agora_config.h"

int agora_rtsa_init(agora_config_t *config);
void agora_rtsa_fini(void);
int agora_rtsa_send_h264_data(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INC_AGORA_RTSA_SDK_H_ */