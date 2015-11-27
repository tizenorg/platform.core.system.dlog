/*
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
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

#ifndef _LOGGER_IOCTL_H
#define _LOGGER_IOCTL_H

#include <sys/ioctl.h>

#define __LOGGERIO      0xAE

#define LOGGER_GET_LOG_BUF_SIZE         _IO(__LOGGERIO, 1) /* size of log */
#define LOGGER_GET_LOG_LEN              _IO(__LOGGERIO, 2) /* used log len */
#define LOGGER_GET_NEXT_ENTRY_LEN       _IO(__LOGGERIO, 3) /* next entry len */
#define LOGGER_FLUSH_LOG                _IO(__LOGGERIO, 4) /* flush log */

#endif /* _LOGGER_IOCTL_H */
