/*
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file	dlog-internal.h
 * @version	0.4
 * @brief	This file is the header file of interface of dlog.
 */
#ifndef	_DLOG_INTERNAL_H_
#define	_DLOG_INTERNAL_H_

#include <tizen_error.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#ifndef __MODULE__
#define __MODULE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/**
 * @internal
 * @brief log id
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @elseif TV 3.0 @endif
 */
typedef enum {
    LOG_ID_MAIN = 0,
	LOG_ID_RADIO,
	LOG_ID_SYSTEM,
	LOG_ID_APPS,
	LOG_ID_MAX
} log_id_t;

static inline int __dlog_no_print(const char *fmt __attribute__((unused)), ...) { return 0; }

#define CONDITION(cond)     (__builtin_expect((cond) != 0, 0))
#define NOP(...) ({ do { __dlog_no_print(__VA_ARGS__); } while (0); })

// Macro inner work---------------------------------------------------------------
#undef LOG_
#define LOG_(id, prio, tag, fmt, arg...) \
	({ do { \
		__dlog_print(id, prio, tag, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##arg); \
	} while (0); })

#undef SECURE_LOG_
#define SECURE_LOG_(id, prio, tag, fmt, arg...) \
	({ do { \
		__dlog_print(id, prio, tag, "%s: %s(%d) > [SECURE_LOG] " fmt, __MODULE__, __func__, __LINE__, ##arg); \
	} while (0); })

// ---------------------------------------------------------------------
/**
 * @internal
 * @brief For Secure Log.
 * @remarks Normally we strip Secure log from release builds.
 * Please use this macros.
 */
/**
 * @internal
 * @brief For Application and etc.
 * @details Simplified macro to send a main log message using the current LOG_TAG.
 * Example:
 *  SECURE_LOGD("app debug %d", num);
 *  SECURE_LOGE("app error %d", num);
 */
#define SECURE_LOGD(format, arg...) SECURE_LOG_(LOG_ID_MAIN, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define SECURE_LOGI(format, arg...) SECURE_LOG_(LOG_ID_MAIN, DLOG_INFO, LOG_TAG, format, ##arg)
#define SECURE_LOGW(format, arg...) SECURE_LOG_(LOG_ID_MAIN, DLOG_WARN, LOG_TAG, format, ##arg)
#define SECURE_LOGE(format, arg...) SECURE_LOG_(LOG_ID_MAIN, DLOG_ERROR, LOG_TAG, format, ##arg)
#define SECURE_LOGF(format, arg...) SECURE_LOG_(LOG_ID_MAIN, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @brief For Framework and system etc.
 * @details Simplified macro to send a system log message using the current LOG_TAG.
 * Example:
 *  SECURE_SLOGD("system debug %d", num);
 *  SECURE_SLOGE("system error %d", num);
 */
#define SECURE_SLOGD(format, arg...) SECURE_LOG_(LOG_ID_SYSTEM, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define SECURE_SLOGI(format, arg...) SECURE_LOG_(LOG_ID_SYSTEM, DLOG_INFO, LOG_TAG, format, ##arg)
#define SECURE_SLOGW(format, arg...) SECURE_LOG_(LOG_ID_SYSTEM, DLOG_WARN, LOG_TAG, format, ##arg)
#define SECURE_SLOGE(format, arg...) SECURE_LOG_(LOG_ID_SYSTEM, DLOG_ERROR, LOG_TAG, format, ##arg)
#define SECURE_SLOGF(format, arg...) SECURE_LOG_(LOG_ID_SYSTEM, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @brief For Modem and radio etc.
 * @details Simplified macro to send a radio log message using the current LOG_TAG.
 * Example:
 *  SECURE_RLOGD("radio debug %d", num);
 *  SECURE_RLOGE("radio error %d", num);
 */
#define SECURE_RLOGD(format, arg...) SECURE_LOG_(LOG_ID_RADIO, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define SECURE_RLOGI(format, arg...) SECURE_LOG_(LOG_ID_RADIO, DLOG_INFO, LOG_TAG, format, ##arg)
#define SECURE_RLOGW(format, arg...) SECURE_LOG_(LOG_ID_RADIO, DLOG_WARN, LOG_TAG, format, ##arg)
#define SECURE_RLOGE(format, arg...) SECURE_LOG_(LOG_ID_RADIO, DLOG_ERROR, LOG_TAG, format, ##arg)
#define SECURE_RLOGF(format, arg...) SECURE_LOG_(LOG_ID_RADIO, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @brief For Tizen OSP Application macro.
 */
#define SECURE_ALOGD(format, arg...) SECURE_LOG_(LOG_ID_APPS, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define SECURE_ALOGI(format, arg...) SECURE_LOG_(LOG_ID_APPS, DLOG_INFO, LOG_TAG, format, ##arg)
#define SECURE_ALOGW(format, arg...) SECURE_LOG_(LOG_ID_APPS, DLOG_WARN, LOG_TAG, format, ##arg)
#define SECURE_ALOGE(format, arg...) SECURE_LOG_(LOG_ID_APPS, DLOG_ERROR, LOG_TAG, format, ##arg)
#define SECURE_ALOGF(format, arg...) SECURE_LOG_(LOG_ID_APPS, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @details If you want use redefined macro.
 * You can use this macro.
 * This macro need priority and tag arguments.
 */
#define SECURE_LOG(priority, tag, format, arg...) SECURE_LOG_(LOG_ID_MAIN, D##priority, tag, format, ##arg)
#define SECURE_SLOG(priority, tag, format, arg...) SECURE_LOG_(LOG_ID_SYSTEM, D##priority, tag, format, ##arg)
#define SECURE_RLOG(priority, tag, format, arg...) SECURE_LOG_(LOG_ID_RADIO, D##priority, tag, format, ##arg)
#define SECURE_ALOG(priority, tag, format, arg...) SECURE_LOG_(LOG_ID_APPS, D##priority, tag, format, ##arg)

/**
 * @internal
 * @brief For Application and etc.
 * @details Simplified macro to send a main log message using the current LOG_TAG.
 * Example:
 *  LOGD("app debug %d", num);
 *  LOGE("app error %d", num);
 */
#define LOGD(format, arg...) LOG_(LOG_ID_MAIN, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define LOGI(format, arg...) LOG_(LOG_ID_MAIN, DLOG_INFO, LOG_TAG, format, ##arg)
#define LOGW(format, arg...) LOG_(LOG_ID_MAIN, DLOG_WARN, LOG_TAG, format, ##arg)
#define LOGE(format, arg...) LOG_(LOG_ID_MAIN, DLOG_ERROR, LOG_TAG, format, ##arg)
#define LOGF(format, arg...) LOG_(LOG_ID_MAIN, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @brief For Framework and system etc.
 * @details Simplified macro to send a system log message using the current LOG_TAG.
 * Example:
 *  SLOGD("system debug %d", num);
 *  SLOGE("system error %d", num);
 */
#define SLOGD(format, arg...) LOG_(LOG_ID_SYSTEM, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define SLOGI(format, arg...) LOG_(LOG_ID_SYSTEM, DLOG_INFO, LOG_TAG, format, ##arg)
#define SLOGW(format, arg...) LOG_(LOG_ID_SYSTEM, DLOG_WARN, LOG_TAG, format, ##arg)
#define SLOGE(format, arg...) LOG_(LOG_ID_SYSTEM, DLOG_ERROR, LOG_TAG, format, ##arg)
#define SLOGF(format, arg...) LOG_(LOG_ID_SYSTEM, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @brief For Modem and radio etc.
 * @details Simplified macro to send a radio log message using the current LOG_TAG.
 * Example:
 *  RLOGD("radio debug %d", num);
 *  RLOGE("radio error %d", num);
 */
#define RLOGD(format, arg...) LOG_(LOG_ID_RADIO, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define RLOGI(format, arg...) LOG_(LOG_ID_RADIO, DLOG_INFO, LOG_TAG, format, ##arg)
#define RLOGW(format, arg...) LOG_(LOG_ID_RADIO, DLOG_WARN, LOG_TAG, format, ##arg)
#define RLOGE(format, arg...) LOG_(LOG_ID_RADIO, DLOG_ERROR, LOG_TAG, format, ##arg)
#define RLOGF(format, arg...) LOG_(LOG_ID_RADIO, DLOG_FATAL, LOG_TAG, format, ##arg)
/**
 * @internal
 * @brief For Tizen OSP Application macro.
 */
#define ALOGD(format, arg...) LOG_(LOG_ID_APPS, DLOG_DEBUG, LOG_TAG, format, ##arg)
#define ALOGI(format, arg...) LOG_(LOG_ID_APPS, DLOG_INFO, LOG_TAG, format, ##arg)
#define ALOGW(format, arg...) LOG_(LOG_ID_APPS, DLOG_WARN, LOG_TAG, format, ##arg)
#define ALOGE(format, arg...) LOG_(LOG_ID_APPS, DLOG_ERROR, LOG_TAG, format, ##arg)
#define ALOGF(format, arg...) LOG_(LOG_ID_APPS, DLOG_FATAL, LOG_TAG, format, ##arg)

/**
 * @internal
 * @brief Basic log message macro
 * @details This macro allows you to specify a priority and a tag
 * if you want to use this macro directly, you must add this messages for unity of messages.
 * (LOG(prio, tag, "%s: %s(%d) > " format, __MODULE__, __func__, __LINE__, ##arg))
 *
 * Example:
 *  #define MYMACRO(prio, tag, format, arg...) \
 *          LOG(prio, tag, format, ##arg)
 *
 *  MYMACRO(LOG_DEBUG, MYTAG, "test mymacro %d", num);
 *
 */
#ifndef LOG
#define LOG(priority, tag, format, arg...) LOG_(LOG_ID_MAIN, D##priority, tag, format, ##arg)
#endif
#define SLOG(priority, tag, format, arg...) LOG_(LOG_ID_SYSTEM, D##priority, tag, format, ##arg)
#define RLOG(priority, tag, format, arg...) LOG_(LOG_ID_RADIO, D##priority, tag, format, ##arg)
#define ALOG(priority, tag, format, arg...) LOG_(LOG_ID_APPS, D##priority, tag, format, ##arg)
/**
 * @internal
 */
#define LOG_VA(priority, tag, fmt, args) \
	vprint_log(D##priority, tag, fmt, args)
#define ALOG_VA(priority, tag, fmt, args) \
	vprint_apps_log(D##priority, tag, fmt, args)
#define RLOG_VA(priority, tag, fmt, args) \
	vprint_radio_log(D##priority, tag, fmt, args)
#define SLOG_VA(priority, tag, fmt, args) \
	vprint_system_log(D##priority, tag, fmt, args)
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

// ---------------------------------------------------------------------
// Don't use below macro no more!! It will be removed -- Verbose and Fatal priority macro will be removed --
#define COMPATIBILITY_ON
#ifdef COMPATIBILITY_ON
/**
 * @internal
 * @breif Conditional Macro.
 * @remarks Don't use this macro. It's just compatibility.
 * It will be deprecated.
 */
#define LOGD_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			LOGD(format, ##arg); \
		} \
	} while (0); })
#define LOGI_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			LOGI(format, ##arg); \
		} \
	} while (0); })
#define LOGW_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			LOGW(format, ##arg); \
		} \
	} while (0); })
#define LOGE_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			LOGE(format, ##arg); \
		} \
	} while (0); })
#define LOGF_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			LOGF(format, ##arg); \
		} \
	} while (0); })
#define SLOGD_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			SLOGD(format, ##arg); \
		} \
	} while (0); })
#define SLOGI_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			SLOGI(format, ##arg); \
		} \
	} while (0); })
#define SLOGW_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			SLOGW(format, ##arg); \
		} \
	} while (0); })
#define SLOGE_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			SLOGE(format, ##arg); \
		} \
	} while (0); })
#define SLOGF_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			SLOGF(format, ##arg); \
		} \
	} while (0); })
#define RLOGD_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			RLOGD(format, ##arg); \
		} \
	} while (0); })
#define RLOGI_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			RLOGI(format, ##arg); \
		} \
	} while (0); })
#define RLOGW_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			RLOGW(format, ##arg); \
		} \
	} while (0); })
#define RLOGE_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			RLOGE(format, ##arg); \
		} \
	} while (0); })
#define RLOGF_IF(cond, format, arg...) \
	({ do { \
		if (CONDITION(cond)) { \
			RLOGF(format, ##arg); \
		} \
	} while (0); })
#define LOG_ON() ({ do { } while (0); })
#define LOGV(format, arg...) NOP(format, ##arg)
#define SLOGV(format, arg...) NOP(format, ##arg)
#define RLOGV(format, arg...) NOP(format, ##arg)
#define ALOGV(format, arg...) NOP(format, ##arg)
#define LOGV_IF(cond, format, arg...) NOP(format, ##arg)
#define SLOGV_IF(cond, format, arg...) NOP(format, ##arg)
#define RLOGV_IF(cond, format, arg...) NOP(format, ##arg)
#define SECLOG(...) ({ do { } while (0); })
#endif
// ---------------------------------------------------------------------

/*
 * The stuff in the rest of this file should not be used directly.
 */
/**
 * @internal
 * @brief     Send log.
 * @details   Use LOG(), SLOG(), RLOG() family
 *            not to use __dlog_print() directly
 * @remarks   Must not use this API directly. use macros instead.
 * @param[in] log_id log device id
 * @param[in] prio priority
 * @param[in] tag tag
 * @param[in] fmt format string
 * @return    Operation result
 * @retval    0>= Success
 * @retval    -1  Error
 * @pre       none
 * @post      none
 * @see       __dlog_print
 * @code
    #define LOG_TAG USR_TAG
    #include<dlog-internal.h>
     __dlog_print(LOG_ID_MAIN, DLOG_INFO, USR_TAG, "you must not use this API directly");
 * @endcode
 */
int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...);

/**
 * @internal
 * @brief     Send log with va_list.
 * @details   Use LOG_VA(), SLOG_VA(), RLOG_VA() family
              not to use __dlog_vprint() directly
 * @remarks   Must not use this API directly. use macros instead.
 * @param[in] log_id log device id
 * @param[in] prio priority
 * @param[in] tag tag
 * @param[in] fmt format string
 * @param[in] ap va_list
 * @return    Operation result
 * @retval    0>= Success
 * @retval    -1  Error
 * @pre       none
 * @post      none
 * @see       __dlog_vprint
 * @code
    #define LOG_TAG USR_TAG
    #include<dlog-internal.h>
     __dlog_vprint(LOG_ID_MAIN, DLOG_INFO, USR_TAG, "you must not use this API directly", ap);
 * @endcode
  */
int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _DLOG_INTERNAL_H_*/
