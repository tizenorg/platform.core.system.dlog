/**
 @ingroup	SLP_PG	
 @defgroup	SLP_PG_Dlog dlog
 @{

<h1 class="pg">Introduction</h1>
Dlog logging service support sending log message to circular log device
	
<h1 class="pg">dlog Architecture</h1>
@image html SLP_Dlog_PG_image001.png System Architecture of dlog

<h1 class="pg">dlog properties</h1>
Sending log message to circular buffer. dlog APIs include Priority and Tag. By using priority and Tag, we can easily filtered messages what we want to see.
	<h2 class="pg">priority</h2>
priority level incdicates the urgency of log message

<table>
<tr>
	<th>Priority</th>
	<th>Description</th>
</tr>
<tr>
	<td>VERBOSE </td>
	<td>Verbose message. - compiled into application and logged at runtime only when debug mode. on release mode, this log message is strip.</td>
</tr>
<tr>
	<td>DEBUG</td>
	<td>Debug messasge. - always compiled into application, but not logged at runtime by default on release mode. on debug mode, this message will be logged at runtime.</td>
</tr>
<tr>
	<td>INFO</td>
	<td>Information message - Normal operational messages. above of this priority will always be logged.</td>
</tr>
<tr>
	<td>WARN</td>
	<td>Warning messages - not an error, but indication that an error will occur if action is not taken</td>
</tr>
<tr>
	<td>ERROR</td>
	<td>Error message - indicate error. </td>
</tr>
<tr>
	<td>FATAL</td>
	<td>Fatal message - Should be corrected immediately ( not used yet )</td>
</tr>
</table>

	<h2 class="pg">Tag</h2>
Used to identify the source of a log message. 
There is no naming limitation, but do not forget that tag is identification of module. Tag must be distinguishable from other tag. 
Simplified macro like LOGV, LOGD, LOGI, LOGW, LOGE uses declared LOG_TAG constant, so declare a LOG_TAG constant before you use dlog macro is a good convention.
@code
#define LOG_TAG "MyApp"
@endcode

<h1 class="pg">list of dlog macro</h1>
Macro name start with LOG prefix is for application. start with SLOG prefix is for framework, start with RLOG prefix is for radio. each macro write log message to separated log device such as main, system, radio. 

<h1 class="pg">sample code</h1>
Using simplified macro with current LOG_TAG

@code
#define LOG_TAG "YOUR_APP"
#include <dlog.h>

int function () {
	LOGD( "debug message from YOUR_APP \n");
	LOGI( "information message from YOUR_APP \n");
	LOGW( "warning message from YOUR_APP \n");
	LOGE( "error message from YOUR_APP \n");
}
@endcode

Using log macro allows you to specify a priority and a tag

@code
#include <dlog.h>

#define TAG_DRM_SVC "drm_svc"
#define TAG_DRM_WM "drm_wm"
#define TAG_DRM_OEM "drm_oem"

int function () {
	LOG(LOG_DEBUG,TAG_DRM_SVC, "information message from drm_svc \n");
	LOG(LOG_WARN,TAG_DRM_WM, "warning message from drm_wm \n");
	LOG(LOG_ERROR,TAG_DRM_OEM, "error message from drm_oem \n");
}
@endcode

Using log macro allows you to pass in a varargs

@code
#include <dlog.h>

#define TAG_DRM_SVC "drm_svc"
#define TAG_DRM_WM "drm_wm"
#define TAG_DRM_OEM "drm_oem"

int function (const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	LOG_VA(LOG_DEBUG,TAG_DRM_WM, fmt, args);
	va_end(ap);
}
@endcode
<h1 class="pg">dlogutil</h1>
	<h2 class="pg">Introduction</h2>
You can use dlogutil command to view and follow the contents of the log buffers. The general usage is :
@code
dlogutil [<option>] ¡¦ [<filter-spec>] ¡¦
@endcode

	<h2 class="pg">Filtering log output</h2>
Every log message has a <I>tag</I> and a <I>priority</I> associated with it.
Filter expression follows this format <B><I>tag:priority</I></B> where <I>tag</I> indicates the tag of interest and <I>priority</I> indicates the minimum level of priority to report for that tag. You can add any number of tag:priority specifications in a single filter expression. 
The tag of a log message is a short indicating the system component from which the message originates 
The <I>priority</I> is one of the following character values, orderd from lowest to highest priority:<br>
V - verbose<br> 
D - debug<br>
I - info<br>
W - warning<br>
E - Error<br>
F - Fatal<br>

for example, if you want to see MyApp tag and above of debug priority, 
@code
# dlogutil MyApp:D
@endcode
if you want to see all log message above of info priority also, 
@code
# dlogutil MyApp:D *:E
@endcode

	<h2 class="pg">List of logutil command options</h2>

<table>
<tr>
	<th>Option</th>
	<th>Description</th>
</tr>
<tr>
	<td>-b <buffer> </td>
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
	<td>-f <filename></td>
	<td>Writes log to <filename>. The default is stdout</td>
</tr>
<tr>
	<td>-g</td>
	<td>Print the size of the specified log buffer and exits.</td>
</tr>
<tr>
	<td>-n <count></td>
	<td>Sets the maximum number of rotated logs to <count>. The default value is 4. Requires the -r option</td>
</tr>
<tr>
	<td>-r <Kbytes></td>
	<td>Rotates the log file every <Kbytes> of output. The default value is 16. Requires the -f option.</td>
</tr>
<tr>
	<td>-s</td>
	<td>Sets the default filter spec to silent.</td>
</tr>
<tr>
	<td>-v <format></td>
	<td>Sets the output format for log messages. The default is brief format. </td>
</tr>

</table>



@}
**/
