/*
 * Copyright (C) 2005 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 

/** 
 * @file	dlog.h
 * @version	0.4
 * @brief	This file is the header file of interface of dlog.
 */


/**
 * @addtogroup APPLICATION_FRAMEWORK
 * @{
 *
 * @defgroup dlog dlog
 * @addtogroup dlog
 * @{

 */


#ifndef	_DLOG_H_
#define	_DLOG_H_

#include<stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Normally we strip LOGV (VERBOSE messages) from release builds.
 */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

/*
 * log priority values, in ascending priority order.
 */
typedef enum {
	DLOG_UNKNOWN = 0,
	DLOG_DEFAULT,
	DLOG_VERBOSE,
	DLOG_DEBUG,
	DLOG_INFO,
	DLOG_WARN,
	DLOG_ERROR,
	DLOG_FATAL,
	DLOG_SILENT,
} log_priority;

typedef enum {
    LOG_ID_MAIN = 0,
    LOG_ID_RADIO,
    LOG_ID_SYSTEM,
    LOG_ID_APPS,
    LOG_ID_MAX
} log_id_t;

#define CONDITION(cond)     (__builtin_expect((cond)!=0, 0))

// ---------------------------------------------------------------------

/**
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef LOGV
#if LOG_NDEBUG
#define LOGV(...)   ((void)0)
#else
#define LOGV(...) ((void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))

#endif
#endif

/**
 * Simplified macro to send a conditional verbose log message using the current LOG_TAG.
 */
#ifndef LOGV_IF
#if LOG_NDEBUG
#define LOGV_IF(cond, ...)   ((void)0)
#else
#define LOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/**
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef LOGD
#define LOGD(...) ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional debug log message using the current LOG_TAG.
 */
#ifndef LOGD_IF
#define LOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/**
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef LOGI
#define LOGI(...) ((void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional info log message using the current LOG_TAG.
 */
#ifndef LOGI_IF
#define LOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/**
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef LOGW
#define LOGW(...) ((void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional warning log message using the current LOG_TAG.
 */
#ifndef LOGW_IF
#define LOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/**
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef LOGE
#define LOGE(...) ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional error log message using the current LOG_TAG.
 */
#ifndef LOGE_IF
#define LOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)LOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

// ---------------------------------------------------------------------

/**
 * Simplified macro to send a verbose radio log message using the current LOG_TAG.
 */
#ifndef RLOGV
#if LOG_NDEBUG
#define RLOGV(...)   ((void)0)
#else
#define RLOGV(...) ((void)RLOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif
	
/**
 * Simplified macro to send a conditional verbose radio log message using the current LOG_TAG.
 */	
#ifndef RLOGV_IF
#if LOG_NDEBUG
#define RLOGV_IF(cond, ...)   ((void)0)
#else
#define RLOGV_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)RLOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
#endif
	
/**
 * Simplified macro to send a debug radio log message using the current LOG_TAG.
 */
#ifndef RLOGD
#define RLOGD(...) ((void)RLOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional debug radio log message using the current LOG_TAG.
 */	
#ifndef RLOGD_IF
#define RLOGD_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)RLOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
	
/**
 * Simplified macro to send an info radio log message using the current LOG_TAG.
 */
#ifndef RLOGI
#define RLOGI(...) ((void)RLOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional info radio log message using the current LOG_TAG.
 */
#ifndef RLOGI_IF
#define RLOGI_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)RLOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
	
/**
 * Simplified macro to send a warning radio log message using the current LOG_TAG.
 */
#ifndef RLOGW
#define RLOGW(...) ((void)RLOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional warning radio log message using the current LOG_TAG.
 */
#ifndef RLOGW_IF
#define RLOGW_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)RLOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
	
/**
 * Simplified macro to send an error radio log message using the current LOG_TAG.
 */
#ifndef RLOGE
#define RLOGE(...) ((void)RLOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional error radio log message using the current LOG_TAG.
 */	
#ifndef RLOGE_IF
#define RLOGE_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)RLOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif

// ---------------------------------------------------------------------

/**
 * Simplified macro to send a verbose system log message using the current LOG_TAG.
 */
#ifndef SLOGV
#if LOG_NDEBUG
#define SLOGV(...)   ((void)0)
#else
#define SLOGV(...) ((void)SLOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

/**
 * Simplified macro to send a conditional verbose system log message using the current LOG_TAG.
 */
#ifndef SLOGV_IF
#if LOG_NDEBUG
#define SLOGV_IF(cond, ...)   ((void)0)
#else
#define SLOGV_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)SLOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
#endif
	
/**
 * Simplified macro to send a debug system log message using the current LOG_TAG.
 */
#ifndef SLOGD
#define SLOGD(...) ((void)SLOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional debug system log message using the current LOG_TAG.
 */
#ifndef SLOGD_IF
#define SLOGD_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)SLOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
	
/**
 * Simplified macro to send an info system log message using the current LOG_TAG.
 */
#ifndef SLOGI
#define SLOGI(...) ((void)SLOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional info system log message using the current LOG_TAG.
 */
#ifndef SLOGI_IF
#define SLOGI_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)SLOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
	
/**
 * Simplified macro to send a warning system log message using the current LOG_TAG.
 */
#ifndef SLOGW
#define SLOGW(...) ((void)SLOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional warning system log message using the current LOG_TAG.
 */
#ifndef SLOGW_IF
#define SLOGW_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)SLOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif
	
/**
 * Simplified macro to send an error system log message using the current LOG_TAG.
 */
#ifndef SLOGE
#define SLOGE(...) ((void)SLOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional error system log message using the current LOG_TAG.
 */
#ifndef SLOGE_IF
#define SLOGE_IF(cond, ...) \
		( (CONDITION(cond)) \
		? ((void)SLOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
		: (void)0 )
#endif

// ---------------------------------------------------------------------

/**
 * Simplified macro to send a verbose log message using the current LOG_TAG.
 */
#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))

#endif
#endif

/**
 * Simplified macro to send a conditional verbose log message using the current LOG_TAG.
 */
#ifndef ALOGV_IF
#if LOG_NDEBUG
#define ALOGV_IF(cond, ...)   ((void)0)
#else
#define ALOGV_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/**
 * Simplified macro to send a debug log message using the current LOG_TAG.
 */
#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional debug log message using the current LOG_TAG.
 */
#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/**
 * Simplified macro to send an info log message using the current LOG_TAG.
 */
#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional info log message using the current LOG_TAG.
 */
#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/**
 * Simplified macro to send a warning log message using the current LOG_TAG.
 */
#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional warning log message using the current LOG_TAG.
 */
#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/**
 * Simplified macro to send an error log message using the current LOG_TAG.
 */
#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

/**
 * Simplified macro to send a conditional error log message using the current LOG_TAG.
 */
#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
    ( (CONDITION(cond)) \
    ? ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
// TODO : support fatal log

// ---------------------------------------------------------------------

/**
 * Basic log message macro that allows you to specify a priority and a tag
 *
 * Example:
 *  LOG(DLOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 * 
 * Future work : we want to put filename and line number automatically only when debug build mode 
 */
#ifndef LOG
#define LOG(priority, tag, ...) \
	print_log(D##priority, tag, __VA_ARGS__)
#endif

/**
 * Log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef LOG_VA
#define LOG_VA(priority, tag, fmt, args) \
    vprint_log(D##priority, tag, fmt, args)
#endif

#ifndef ALOG
#define ALOG(priority, tag, ...) \
	print_apps_log(D##priority, tag, __VA_ARGS__)
#endif
/**
 * Log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef ALOG_VA
#define ALOG_VA(priority, tag, fmt, args) \
    vprint_apps_log(D##priority, tag, fmt, args)
#endif

/**
 * Basic radio log macro that allows you to specify a priority and a tag.
 */
#ifndef RLOG
#define RLOG(priority, tag, ...) \
	print_radio_log(D##priority, tag, __VA_ARGS__)
#endif

/**
 * Radio log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef RLOG_VA
#define RLOG_VA(priority, tag, fmt, args) \
    vprint_radio_log(D##priority, tag, fmt, args)
#endif

/**
 * Basic system log macro that allows you to specify a priority and a tag.
 */
#ifndef SLOG
#define SLOG(priority, tag, ...) \
    print_system_log(D##priority, tag, __VA_ARGS__)
#endif

/**
 * System log macro that allows you to pass in a varargs ("args" is a va_list).
 */
#ifndef SLOG_VA
#define SLOG_VA(priority, tag, fmt, args) \
    vprint_system_log(D##priority, tag, fmt, args)
#endif


/*
 * ===========================================================================
 *
 * The stuff in the rest of this file should not be used directly.
 */

#define print_apps_log(prio, tag, fmt...) \
	__dlog_print(LOG_ID_APPS, prio, tag, fmt)

#define vprint_apps_log(prio, tag, fmt...) \
	__dlog_vprint(LOG_ID_APPS, prio, tag, fmt)

#define print_log(prio, tag, fmt...) \
	__dlog_print(LOG_ID_MAIN, prio, tag, fmt)

#define vprint_log(prio, tag, fmt...) \
	__dlog_vprint(LOG_ID_MAIN, prio, tag, fmt)
	
#define print_radio_log(prio, tag, fmt...)\
	__dlog_print(LOG_ID_RADIO, prio, tag, fmt)

#define vprint_radio_log(prio, tag, fmt...) \
	__dlog_vprint(LOG_ID_RADIO, prio, tag, fmt)

#define print_system_log(prio, tag, fmt...)\
	__dlog_print(LOG_ID_SYSTEM, prio, tag, fmt)

#define vprint_system_log(prio, tag, fmt...) \
	__dlog_vprint(LOG_ID_SYSTEM, prio, tag, fmt)

/**
 * @brief		send log. must specify log_id ,priority, tag and format string.
 * @pre		none
 * @post		none
 * @see		__dlog_vprint
 * @remarks	you must not use this API directly. use macros instead.
 * @param[in]	log_id	log device id
 * @param[in]	prio	priority
 * @param[in]	tag	tag
 * @param[in]	fmt	format string 
 * @return			Operation result
 * @retval		0>=	Success
 * @retval              -1	Error
 * @code
// you have to use LOG(), SLOG(), RLOG() family not to use __dlog_print() directly
// so below example is just for passing Documentation Verification !!!
#define LOG_TAG USR_TAG
#include<dlog.h>
 __dlog_print(LOG_ID_MAIN, DLOG_INFO, USR_TAG, "you must not use this API directly");
 * @endcode
 */
int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...);

/**
 * @brief		send log with va_list. must specify log_id ,priority, tag and format string.
 * @pre		none
 * @post		none
 * @see		__dlog_print 
 * @remarks	you must not use this API directly. use macros instead. 
 * @param[in]	log_id	log device id
 * @param[in]	prio	priority
 * @param[in]	tag	tag
 * @param[in]	fmt	format string 
 * @param[in]	ap	va_list 
 * @return			Operation result
 * @retval		0>=	Success
 * @retval              -1	Error
 * @code
 // you have to use LOG_VA(), SLOG_VA(), RLOG_VA() family not to use __dlog_print() directly
 // so below example is just for passing Documentation Verification !!!
#define LOG_TAG USR_TAG
#include<dlog.h>
  __dlog_vprint(LOG_ID_MAIN, DLOG_INFO, USR_TAG, "you must not use this API directly", ap);
 * @endcode
  */
int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/** @} */
/** @} */

#endif /* _DLOG_H_*/

