/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Apr 6th, 2025
 * Module:	AOSL json program API definition file
 *
 *
 * This is a part of the Advanced Operating System Layer.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AOSL_JPROG_H__
#define __AOSL_JPROG_H__


#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_data.h>
#include <api/aosl_thread.h>
#include <api/aosl_ref.h>
#include <api/aosl_module.h>
#include <api/aosl_json.h>
#include <api/aosl_sbus.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef int (*aosl_jprog_entry_t) (int cmd, aosl_data_t input, aosl_data_t output);

#define AOSL_JPROG_MODULE(mod, entry) \
	static int __##mod##_jprog_entry (uintptr_t argc, uintptr_t argv []) \
	{ \
		return (entry) ((int)argv [0], (aosl_data_t)argv [1], (aosl_data_t)argv [2]); \
	} \
	AOSL_DEFINE_MODULE (mod, __##mod##_jprog_entry)


typedef int (*aosl_jprog_func_t) (aosl_data_t input, aosl_data_t output);

extern __so_api__ int aosl_jprog_func_register (const char *name, aosl_jprog_func_t func);
extern __so_api__ int aosl_jprog_func_unregister (const char *name);

#define AOSL_JPROG_FUNC(module, func) AOSL_DEFINE_NAMED_ENTRY(jprog_func, module##$##func, func)

#define AOSL_JPROG_OPEN_NO_LOAD 0x00000001 /* open only, do not load the json program */

/**
 * Open and initialize the json program in the specified file.
 * Parameters:
 *   jsonfile: the json based program file;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *        ...: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jprog_open_file (const char *jsonfile, int flags, int argc, ...);

/**
 * Open and initialize the json program in the specified file.
 * Parameters:
 *   jsonfile: the json based program file;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       args: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jprog_open_file_args (const char *jsonfile, int flags, int argc, va_list args);

/**
 * Open and initialize the json program in the specified file.
 * Parameters:
 *   jsonfile: the json based program file;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       argv: the arguments vector;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jprog_open_file_argv (const char *jsonfile, int flags, int argc, const char *argv []);

/**
 * Open and initialize the json program in the specified string.
 * Parameters:
 *    jsonstr: the json based program string;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *        ...: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jprog_open_string (const char *jsonstr, int flags, int argc, ...);

/**
 * Open and initialize the json program in the specified string.
 * Parameters:
 *    jsonstr: the json based program string;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       args: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jprog_open_string_args (const char *jsonstr, int flags, int argc, va_list args);

/**
 * Open and initialize the json program in the specified string.
 * Parameters:
 *    jsonstr: the json based program string;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       argv: the arguments vector;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jprog_open_string_argv (const char *jsonstr, int flags, int argc, const char *argv []);

/**
 * Add a boolean global variable to the json program.
 * Parameters:
 *   jprog: the json program object;
 *   name: the name of the global variable;
 *   value: the value of the global variable;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_globals_add_bool (aosl_ref_t jprog, const char *name, int value);

/**
 * Add an integer global variable to the json program.
 * Parameters:
 *   jprog: the json program object;
 *   name: the name of the global variable;
 *   value: the value of the global variable;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_globals_add_int (aosl_ref_t jprog, const char *name, int64_t value);

/**
 * Add a double global variable to the json program.
 * Parameters:
 *   jprog: the json program object;
 *   name: the name of the global variable;
 *   value: the value of the global variable;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_globals_add_double (aosl_ref_t jprog, const char *name, double value);

/**
 * Add a string global variable to the json program.
 * Parameters:
 *   jprog: the json program object;
 *   name: the name of the global variable;
 *   value: the value of the global variable;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_globals_add_string (aosl_ref_t jprog, const char *name, const char *value);

/**
 * Import a json program module from the specified file dynamically.
 * Parameters:
 *      jprog: the json program object;
 *       name: the name of the json program module, unique in the json program;
 *   filename: the json program file;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_import_file (aosl_ref_t jprog, const char *name, const char *filename);

/**
 * Import a json program module from the specified string dynamically.
 * Parameters:
 *      jprog: the json program object;
 *       name: the name of the json program module, unique in the json program;
 *        str: the json program string;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_import_string (aosl_ref_t jprog, const char *name, const char *str);

/**
 * Load the specified json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 * Remarks:
 *     This function loads the json program, only needed when the json program is opened with AOSL_JPROG_OPEN_NO_LOAD flag.
 **/
extern __aosl_api__ int aosl_jprog_load (aosl_ref_t jprog);

/**
 * Run the specified json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_run (aosl_ref_t jprog);

/**
 * Evaluate the specified json string.
 * Parameters:
 *      jprog: the json program object;
 *    jsonstr: the json string to execute;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_eval (aosl_ref_t jprog, const char *jsonstr);

/**
 * Find the json object specified by the path in the specified json program.
 * Parameters:
 *      jprog: the json program object;
 *      jpath: the path of the json object;
 * Return value:
 *     non-NULL: the json object specified by the path;
 *         NULL: failed with aosl_errno set;
 **/
extern __aosl_api__ aosl_jobj_t aosl_jprog_find (aosl_ref_t jprog, const char *jpath);

/**
 * Pause the specified json program for debugging purpose etc.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_pause (aosl_ref_t jprog);

/**
 * Suspend the specified json thread.
 * Parameters:
 *      jprog: the json program object;
 *       thrd: the json thread;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: current suspend count;
 **/
extern __aosl_api__ int aosl_jthread_suspend (aosl_ref_t jprog, aosl_thread_t thrd);

/**
 * Check if the specified json program is started.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *     !=0: the json program is started;
 *     ==0: the json program is not started;
 **/
extern __aosl_api__ int aosl_jprog_started (aosl_ref_t jprog);

/**
 * Check if the specified json program is stopped.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *     !=0: the json program is stopped;
 *     ==0: the json program is running;
 **/
extern __aosl_api__ int aosl_jprog_stopped (aosl_ref_t jprog);


/**
 * Resume the specified json thread.
 * Parameters:
 *      jprog: the json program object;
 *       thrd: the json thread;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: current suspend count;
 **/
extern __aosl_api__ int aosl_jthread_resume (aosl_ref_t jprog, aosl_thread_t thrd);

/**
 * Kill the specified json thread.
 * Parameters:
 *      jprog: the json program object;
 *       thrd: the json thread;
 * Return value:
 *      <0: indicate some error occurs;
 *       0: current thread has been killed already;
 *       1: current thread is killed now;
 **/
extern __aosl_api__ int aosl_jthread_kill (aosl_ref_t jprog, aosl_thread_t thrd);

/**
 * Resume the specified stopped json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jprog_resume (aosl_ref_t jprog);

typedef struct {
	aosl_thread_t thrd;
	const char *stmt_name;
} aosl_jthread_state_t;

/**
 * Get the state of the threads in the specified json program.
 * Parameters:
 *      jprog: the json program object;
 *     states: the array of states;
 *      count: the size of the array;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: the total number of threads;
 **/
extern __aosl_api__ int aosl_jprog_get_threads (aosl_ref_t jprog, aosl_jthread_state_t *states, size_t count);

/**
 * json program debugger open and initialize the json program in the specified file.
 * Parameters:
 *   jsonfile: the json based program file;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *        ...: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jpdb_open_file (const char *jsonfile, int flags, int argc, ...);

/**
 * json program debugger open and initialize the json program in the specified file.
 * Parameters:
 *   jsonfile: the json based program file;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       args: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jpdb_open_file_args (const char *jsonfile, int flags, int argc, va_list args);

/**
 * json program debugger open and initialize the json program in the specified file.
 * Parameters:
 *   jsonfile: the json based program file;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       argv: the arguments vector;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jpdb_open_file_argv (const char *jsonfile, int flags, int argc, const char *argv []);

/**
 * json program debugger open and initialize the json program in the specified string.
 * Parameters:
 *    jsonstr: the json based program string;
 *      flags: the flags for open the json program;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jpdb_open_string (const char *jsonstr, int flags, int argc, ...);

/**
 * json program debugger open and initialize the json program in the specified string.
 * Parameters:
 *    jsonstr: the json based program string;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       args: the arguments;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jpdb_open_string_args (const char *jsonstr, int flags, int argc, va_list args);

/**
 * json program debugger open and initialize the json program in the specified string.
 * Parameters:
 *    jsonstr: the json based program string;
 *      flags: the flags for open the json program;
 *       argc: the number of arguments;
 *       argv: the arguments vector;
 * Return value:
 *     non-NULL: the json program object;
 *         NULL: failed with aosl_errno set;
 * Remarks:
 *     This function will load the json program automatically if not specified
 *     AOSL_JPROG_OPEN_NO_LOAD, so no need to call aosl_jprog_load() later for
 *     this case.
 **/
extern __aosl_api__ aosl_ref_t aosl_jpdb_open_string_argv (const char *jsonstr, int flags, int argc, const char *argv []);

/**
 * Attach the json program debugger to the specified json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_attach (aosl_ref_t jprog);

typedef struct {
	const char *stmt_name;
	const char *file_name;
} aosl_jframe_t;

/**
 * Get the backtrace of the specified json thread.
 * Parameters:
 *      jprog: the json program object;
 *       thrd: the json thread;
 *     frames: the array of frames;
 *      count: the size of the array;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: the total number of frames;
 **/
extern __aosl_api__ int aosl_jthread_backtrace (aosl_ref_t jprog, aosl_thread_t thrd, aosl_jframe_t *frames, size_t count);

/**
 * Step one statement of the specified json program for debugging, step into the function calls.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_step (aosl_ref_t jprog);

/**
 * Step one statement of the specified json program for debugging, proceed through the function calls.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_next (aosl_ref_t jprog);

/**
 * Run the specified debugging json program until the current frame finished.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_finish (aosl_ref_t jprog);

/**
 * Continue execution of all threads in the specified json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_continue (aosl_ref_t jprog);

/**
 * Insert a breakpoint at the specified statement of the json program for debugging.
 * Parameters:
 *      jprog: the json program object;
 *      jpath: the json path of the statement to insert the breakpoint;
 *   thrd_idx: the index of the thread to insert the breakpoint, -1 for all threads;
 *       cond: the triggering condition of the breakpoint, NULL for no condition;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 * Remarks:
 *     The condition is a valid json program expression, the breakpoint will be triggered
 *     only if the condition is true.
 **/
extern __aosl_api__ int aosl_jpdb_breakpoint_add (aosl_ref_t jprog, const char *jpath, int thrd_idx, const char *cond);

/**
 * Disable a breakpoint at the specified statement of the json program for debugging.
 * Parameters:
 *      jprog: the json program object;
 *        idx: the index of the breakpoint;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_breakpoint_disable (aosl_ref_t jprog, int idx);

/**
 * Enable a breakpoint at the specified statement of the json program for debugging.
 * Parameters:
 *      jprog: the json program object;
 *        idx: the index of the breakpoint;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_breakpoint_enable (aosl_ref_t jprog, int idx);

/**
 * Delete a breakpoint at the specified statement of the json program.
 * Parameters:
 *      jprog: the json program object;
 *        idx: the index of the breakpoint;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_breakpoint_del (aosl_ref_t jprog, int idx);

typedef enum {
	AOSL_JPDB_CP_TYPE_BP, /* breakpoint */
	AOSL_JPDB_CP_TYPE_RW, /* read watchpoint */
	AOSL_JPDB_CP_TYPE_WW, /* write watchpoint */
} aosl_jpdb_cp_type_t;

typedef struct {
	const char *stmt_name;
	const char *content;
	aosl_jpdb_cp_type_t type;
	int idx;
	int enabled;
	int hits;
} aosl_jpdb_checkpoint_t;

/**
 * Get the breakpoints of the json program.
 * Parameters:
 *      jprog: the json program object;
 *        bps: the array of breakpoints;
 *      count: the size of the array;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: the total number of breakpoints;
 **/
extern __aosl_api__ int aosl_jpdb_get_breakpoints (aosl_ref_t jprog, aosl_jpdb_checkpoint_t *bps, size_t count);

#define AOSL_JPDB_WP_T_RD 0x00000001 /* read watchpoint */
#define AOSL_JPDB_WP_T_WR 0x00000002 /* write watchpoint */
#define AOSL_JPDB_WP_T_RW (AOSL_JPDB_WP_T_RD | AOSL_JPDB_WP_T_WR) /* read/write watchpoint */

/**
 * Insert a watchpoint at the specified statement of the json program for debugging.
 * Parameters:
 *      jprog: the json program object;
 *       type: the bitmask of type of the watchpoint, AOSL_JPDB_WP_T_RD, AOSL_JPDB_WP_T_WR;
 *      jpath: the json path of the statement to insert the watchpoint;
 *   thrd_idx: the index of the thread to insert the watchpoint, -1 for all threads;
 *       cond: the triggering condition of the breakpoint, NULL for no condition;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 * Remarks:
 *     The condition is a valid json program expression, the watchpoint will be triggered
 *     only if the condition is true.
 **/
extern __aosl_api__ int aosl_jpdb_watchpoint_add (aosl_ref_t jprog, int type, const char *jpath, int thrd_idx, const char *cond);

/**
 * Disable a watchpoint at the specified statement of the json program.
 * Parameters:
 *      jprog: the json program object;
 *       type: the bitmask of type of the watchpoint, AOSL_JPDB_WP_T_RD, AOSL_JPDB_WP_T_WR;
 *        idx: the index of the watchpoint;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_watchpoint_disable (aosl_ref_t jprog, int type, int idx);

/**
 * Enable a watchpoint at the specified statement of the json program.
 * Parameters:
 *      jprog: the json program object;
 *       type: the bitmask of type of the watchpoint, AOSL_JPDB_WP_T_RD, AOSL_JPDB_WP_T_WR;
 *        idx: the index of the watchpoint;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_watchpoint_enable (aosl_ref_t jprog, int type, int idx);

/**
 * Delete a watchpoint at the specified statement of the json program.
 * Parameters:
 *      jprog: the json program object;
 *       type: the bitmask of type of the watchpoint, AOSL_JPDB_WP_T_RD, AOSL_JPDB_WP_T_WR;
 *        idx: the index of the watchpoint;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_watchpoint_del (aosl_ref_t jprog, int type, int idx);

/**
 * Get the read watchpoints of the json program.
 * Parameters:
 *      jprog: the json program object;
 *       type: the type of the watchpoints, AOSL_JPDB_WP_T_RD, AOSL_JPDB_WP_T_WR, AOSL_JPDB_WP_T_RW;
 *        wps: the array of watchpoints;
 *      count: the size of the array;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: the total number of breakpoints;
 **/
extern __aosl_api__ int aosl_jpdb_get_watchpoints (aosl_ref_t jprog, int type, aosl_jpdb_checkpoint_t *wps, size_t count);

/**
 * Get the current thread of the json program debugger.
 * Parameters:
 *      jprog: the json program object;
 *     thrd_p: the pointer to the thread;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_thread_get (aosl_ref_t jprog, aosl_thread_t *thrd_p);

/**
 * Set the current thread of the json program debugger.
 * Parameters:
 *      jprog: the json program object;
 *       thrd: the thread;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_thread_set_by_thrd (aosl_ref_t jprog, aosl_thread_t thrd);

/**
 * Set the current thread of the json program debugger.
 * Parameters:
 *      jprog: the json program object;
 *       thrd: the thread;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_thread_set_by_idx (aosl_ref_t jprog, int idx);


typedef enum {
	AOSL_JPDB_INFERIOR_EVENT_PAUSE,
	AOSL_JPDB_INFERIOR_EVENT_BREAKPOINT,
	AOSL_JPDB_INFERIOR_EVENT_WATCHPOINT_READ,
	AOSL_JPDB_INFERIOR_EVENT_WATCHPOINT_WRITE,
	AOSL_JPDB_INFERIOR_EVENT_STEP,
	AOSL_JPDB_INFERIOR_EVENT_NEXT,
	AOSL_JPDB_INFERIOR_EVENT_FINISH,
} aosl_jpdb_inferior_event_type_t;

typedef struct {
	aosl_jpdb_inferior_event_type_t stop_reason;
	uint32_t thrd_idx;
	const char *stmt_name;
} aosl_jpdb_inferior_event_t;

/**
 * Get the inferior event of the json program debugger.
 * Parameters:
 *      jprog: the json program object;
 *      event: the pointer to the event;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 * Remarks:
 *     This function will block until the json program is stopped.
 **/
extern __aosl_api__ int aosl_jpdb_inferior_event_get (aosl_ref_t jprog, aosl_jpdb_inferior_event_t *event);

#if defined (__linux__) || defined (__APPLE__)
typedef int aosl_jpdb_inferior_notif_obj_t;
#elif defined (_WIN32)
typedef HANDLE aosl_jpdb_inferior_notif_obj_t;
#else
typedef void *aosl_jpdb_inferior_notif_obj_t;
#endif

/**
 * Get the inferior event of the json program debugger.
 * Parameters:
 *      jprog: the json program object;
 *  notif_obj: the pointer to save the notification object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 * Remarks:
 *     This function will block until the json program is stopped.
 **/
extern __aosl_api__ int aosl_jpdb_inferior_notif_obj_get (aosl_ref_t jprog, aosl_jpdb_inferior_notif_obj_t *notif);

/**
 * Detach the json program debugger from the specified json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *      <0: indicate some error occurs;
 *     >=0: successful;
 **/
extern __aosl_api__ int aosl_jpdb_detach (aosl_ref_t jprog);

/**
 * Kill the json program.
 * Parameters:
 *      jprog: the json program object;
 * Return value:
 *     None.
 **/
extern __aosl_api__ void aosl_jprog_kill (aosl_ref_t jprog);


#ifdef __cplusplus
}
#endif



#endif /* __AOSL_JPROG_H__ */