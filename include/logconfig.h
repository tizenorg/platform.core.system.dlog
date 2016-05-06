/*
 * DLOG
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
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

#ifndef _LOGCONFIG_H_
#define _LOGCONFIG_H_

#define CONFIG_FILENAME TZ_SYS_ETC"/dlog.conf"

#define MAX_CONF_KEY_LEN   32
#define MAX_CONF_VAL_LEN   256
#define MAX_CONF_ENTRY_LEN (MAX_CONF_KEY_LEN + MAX_CONF_VAL_LEN + 2) // +2 for the delimiter and newline

struct log_conf_entry;

struct log_config {
	struct log_conf_entry *begin;
	struct log_conf_entry *last;
};

int log_config_set (struct log_config* config, const char* key, const char* value);
const char* log_config_get (struct log_config* config, const char* key);
int log_config_read (struct log_config* config);
int log_config_write(struct log_config* config);
void log_config_free (struct log_config* config);

void log_config_print_out (struct log_config* config);
int log_config_print_key (struct log_config* config, const char* key);
void log_config_push (struct log_config* config, const char* key, const char* value);
int log_config_remove (struct log_config* config, const char* key);
int log_config_foreach (struct log_config* config, int (*func)(const char* key, const char* value));

#endif /* _LOGCONFIG_H_ */
