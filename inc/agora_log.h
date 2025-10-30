#ifndef AGORA_LOG_H_
#define AGORA_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define LOGD(TAG, fmt, ...) fprintf(stdout, "\033[0m\033[30;47m[Dbg]\033[0m%s[%s:%u]" fmt "\n", TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGT(TAG, fmt, ...) fprintf(stdout, "\033[0m\033[30;42m[Inf]\033[0m%s[%s:%u]" fmt "\n", TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__) 
#define LOGW(TAG, fmt, ...) fprintf(stdout, "\033[0m\033[30;43m[Wrn]\033[0m%s[%s:%u]" fmt "\n", TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define LOGE(TAG, fmt, ...) fprintf(stdout, "\033[0m\033[30;41m[Err]\033[0m%s[%s:%u]" fmt "\n", TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* AGORA_LOG_H_ */