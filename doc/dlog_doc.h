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

<h2 class="pg">Macro example for useful usage</h2>
The dlog APIs can be used by defining own macros. The macros can be defined like below examples.
Thus, developers can use appropriate method as a matter of convenience.

@code
#undef LOG_TAG
#define LOG_TAG "APP_TAG"

#define LOGI(fmt, arg...) \
	({ do { \
		dlog_print(DLOG_INFO, LOG_TAG, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##arg); \
	} while (0); })
#define LOGW(fmt, arg...) \
	({ do { \
		dlog_print(DLOG_WARN, LOG_TAG, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##arg); \
	} while (0); })
#define LOGE(fmt, arg...) \
	({ do { \
		dlog_print(DLOG_ERROR, LOG_TAG, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, ##arg); \
	} while (0); })
@endcode

<h1 class="pg">dlogutil</h1>
<h2 class="pg">Introduction</h2>
You can use dlogutil command to view and follow the contents of the log buffers. The general usage is :
@code
dlogutil [<option>] ... [<filter-spec>] ...
@endcode

<h2 class="pg">Filtering log output</h2>
Every log message has a <I>tag</I> and a <I>priority</I> associated with it.
Filter expression follows this format <B><I>tag:priority</I></B> where <I>tag</I> indicates the tag of interest and <I>priority</I> indicates the minimum level of priority to report for that tag. You can add any number of tag:priority specifications in a single filter expression.
The tag of a log message is a short indicating the system component from which the message originates
The <I>priority</I> is one of the following character values, orderd from lowest to highest priority:<br>
D - debug<br>
I - info<br>
W - warning<br>
E - Error<br>

for example, if you want to see MY_APP tag and above of debug priority,
@code
# dlogutil MY_APP:D
@endcode
if you want to see all log message above of info priority.
@code
# dlogutil *:I
@endcode

<h2 class="pg">List of logutil command options</h2>

<table>
<tr>
	<th>Option</th>
	<th>Description</th>
</tr>
<tr>
	<td>-b \<buffer\> </td>
	<td>Alternate log buffer. The main buffer is used by default buffer. </td>
</tr>
<tr>
	<td>-c</td>
	<td>Clears the entire log and exits</td>
</tr>
<tr>
	<td>-d</td>
	<td>Dumps the log and exits.</td>
</tr>
<tr>
	<td>-f \<filename\></td>
	<td>Writes log to filename. The default is stdout</td>
</tr>
<tr>
	<td>-g</td>
	<td>Print the size of the specified log buffer and exits.</td>
</tr>
<tr>
	<td>-n \<count\></td>
	<td>Sets the maximum number of rotated logs to count. The default value is 4. Requires the -r option</td>
</tr>
<tr>
	<td>-r \<Kbytes\></td>
	<td>Rotates the log file every Kbytes of output. The default value is 16. Requires the -f option.</td>
</tr>
<tr>
	<td>-s</td>
	<td>Sets the default filter spec to silent.</td>
</tr>
<tr>
	<td>-v \<format\></td>
	<td>Sets the output format for log messages. The default is brief format. </td>
</tr>

</table>
>>>>>>> mobile
*
**/
#endif /* __TIZEN_SYSTEM_DLOG_DOC_H__ */
