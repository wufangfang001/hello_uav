/*************************************************************
 * Author: Lionfore Hao (haolianfu@agora.io)
 * Date  : Jul 21st, 2018
 * Module: AOSL data structure marshalling
 *
 *
 * This is a part of the Advanced Operating System Layer.
 * Copyright (C) 2018 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_MARSHALLING_H__
#define __AOSL_MARSHALLING_H__

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_typed.h>
#include <api/aosl_psb.h>


#ifdef __cplusplus
extern "C" {
#endif


extern __aosl_api__ ssize_t aosl_marshal (const aosl_type_t *type, const void *typed_obj_p, aosl_psb_t *psb);
extern __aosl_api__ ssize_t aosl_unmarshal (const aosl_type_t *type, void *typed_obj_p, const aosl_psb_t *psb);
extern __aosl_api__ ssize_t aosl_marshal_c_str (const char *str, uint32_t nmax, aosl_psb_t *psb);
extern __aosl_api__ ssize_t aosl_unmarshal_c_str (char *str, uint32_t nmax, const aosl_psb_t *psb);
extern __aosl_api__ ssize_t aosl_marshal_size (const aosl_type_t *type, const void *typed_obj_p);


#ifdef __cplusplus
}
#endif


#endif /* __AOSL_MARSHALLING_H__ */