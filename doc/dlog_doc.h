/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TIZEN_SYSTEM_DLOG_DOC_H__
#define __TIZEN_SYSTEM_DLOG_DOC_H__

/**
 * @defgroup CAPI_SYSTEM_DLOG dlog
 * @brief The Dlog API provides functions for sending log output.
 * @ingroup CAPI_SYSTEM_FRAMEWORK
 * @section CAPI_SYSTEM_DLOG_HEADER Required Header
 *   \#include <dlog.h>
 *
 * @section CAPI_SYSTEM_DLOG_OVERVIEW Overview
 *
Sending log message to circular buffer. dlog APIs include Priority and Tag. By using priority and Tag, we can easily filtered messages what we want to see.
<h2 class="pg">priority</h2>
priority level indicates the urgency of log message

<table>
<tr>
	<th>Priority</th>
	<th>Description</th>
</tr>
<tr>
	<td>DLOG_DEBUG</td>
	<td>Debug messasge. - Log message which developer want to check.</td>
</tr>
<tr>
	<td>DLOG_INFO</td>
	<td>Information message - Normal operational messages. above of this priority will always be logged.</td>
</tr>
<tr>
	<td>DLOG_WARN</td>
	<td>Warning messages - Not an error, but indication that an error will occur if action is not taken.</td>
</tr>
<tr>
	<td>DLOG_ERROR</td>
	<td>Error message - Indicate error.</td>
</tr>
</table>
*
**/
#endif /* __TIZEN_SYSTEM_DLOG_DOC_H__ */
