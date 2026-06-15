/*************************************************************
 * Author: Lionfore Hao (haolianfu@agora.io)
 * Date  : Apr 7th, 2025
 * Module: AOSL JSON file relative API declaration file
 *
 *
 * This is a part of the Advanced Operating System Layer.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_JSON_H__
#define __AOSL_JSON_H__

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_typed.h>
#include <api/aosl_data.h>


#ifdef __cplusplus
extern "C" {
#endif

/* aosl json object type */
typedef struct __aosl_json_obj__ *aosl_jobj_t;

/**
 * Get the json object specified by the path from the specified file.
 * Parameters:
 *   jsonfile: the json file;
 *      opath: the path of the json object, NULL or "/" for the root object;
 * Return value:
 *     non-NULL: the json object specified by the path;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_jobj_from_file (const char *jsonfile, const char *opath);

/**
 * Get the json object specified by the path from the specified string.
 * Parameters:
 *    jsonstr: the json string;
 *      opath: the path of the json object, NULL or "/" for the root object;
 * Return value:
 *     non-NULL: the json object specified by the path;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_jobj_from_string (const char *jsonstr, const char *opath);

/**
 * Get the sub json object specified by the path of the specified json object.
 * Parameters:
 *       jobj: the json object whose sub object to get;
 *      opath: the path of the sub json object, must not be NULL;
 * Return value:
 *     non-NULL: the sub json object specified by the path;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_jobj_get (aosl_jobj_t jobj, const char *opath);

/**
 * Get the string value of the specified json object.
 * Parameters:
 *       jobj: the json object;
 * Return value:
 *     non-NULL: the string value of the json object;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ const char *aosl_jobj_string (aosl_jobj_t jobj);

/**
 * Get the typed variable from the specified json object.
 * Parameters:
 *         jobj: the json object;
 *         type: the type of the variable to get;
 *  typed_obj_p: the pointer to the variable to get;
 * Return value:
 *          >=0: the bytes copied for filling the variable;
 *           <0: failed with aosl_errno set;
 **/
extern __aosl_api__ int aosl_jobj_typed_var (aosl_jobj_t jobj, const aosl_type_t *type, void *typed_obj_p);

/**
 * Get the typed variable presented by aosl data object from the specified json object.
 * Parameters:
 *         jobj: the json object;
 *         type: the type of the variable to get;
 * Return value:
 *     non-NULL: the aosl data object created automatically;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     The aosl data object returned by this function must be put
 *     manually when finished using.
 **/
extern __aosl_api__ aosl_data_t aosl_jobj_typed_data (aosl_jobj_t jobj, const aosl_type_t *type);

/**
 * Create a json object from the specified typed variable.
 * Parameters:
 *  typed_obj_p: the typed variable address;
 *         type: the type of the variable;
 * Return value:
 *     non-NULL: the json object created from typed variable;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_jobj_from_typed_var (const void *typed_obj_p, const aosl_type_t *type);

/**
 * Create a json object from the specified typed data.
 * Parameters:
 *         data: the aosl data object which holds the typed variable;
 *         type: the type of the variable;
 * Return value:
 *     non-NULL: the json object created from typed data;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_jobj_from_typed_data (aosl_data_t data, const aosl_type_t *type);

/**
 * Create an empty typed json object.
 * Parameters:
 *         type: the type of the variable;
 * Return value:
 *     non-NULL: the created empty json object;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_typed_jobj (const aosl_type_t *type);

/**
 * Put the specified json object.
 * Parameters:
 *         jobj: the json object to put;
 * Return value:
 *         None.
 * Remarks:
 *     The json object returned by any of these functions must be put
 *     manually by this function when finished using.
 **/
extern __aosl_api__ void aosl_jobj_put (aosl_jobj_t jobj);


#ifdef __cplusplus
}
#endif


#endif /* __AOSL_JSON_H__ */