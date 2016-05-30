#include <logconfig.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef TZ_SYS_ETC
#define TZ_SYS_ETC "/opt/etc"
#endif

const char * get_config_filename (config_type type)
{
	switch (type) {
	case CONFIG_TYPE_COMMON:
		return TZ_SYS_ETC "/dlog.conf";
	case CONFIG_TYPE_KMSG:
		return "/run/dlog.conf";
	default:
		return NULL;
	}
}

struct log_conf_entry {
	char key [MAX_CONF_KEY_LEN];
	char value [MAX_CONF_VAL_LEN];
	struct log_conf_entry * next;
};

const char * log_config_get (struct log_config* c, const char* key)
{
	struct log_conf_entry * e;

	if (!c)
		return NULL;

	e = c->begin;
	while (e)
		if (!strcmp (e->key, key))
			return e->value;
		else
			e = e->next;

	return NULL;
}

int log_config_set (struct log_config* c, const char* key, const char* value)
{
	struct log_conf_entry * e;

	if (!c || !key || !value)
		return 0;

	e = c->begin;
	while (e) {
		if (!strcmp (e->key, key)) {
			strncpy (e->value, value, MAX_CONF_VAL_LEN);
			return 1;
		}
		e = e->next;
	}
	return 0;
}

int log_config_read (struct log_config* config)
{
	int ret = 0;
	char const * override;

	if (!config)
		return 0;

	config->begin = config->last = NULL;

	override = getenv ("DLOG_CONFIG_PATH");
	if (override)
		return log_config_read_file (config, override);
#ifdef DLOG_BACKEND_KMSG
	ret |= log_config_read_file (config, get_config_filename (CONFIG_TYPE_KMSG)); // fixme: ugly
#endif
	ret |= log_config_read_file (config, get_config_filename (CONFIG_TYPE_COMMON));

	return ret;
}

int log_config_read_file (struct log_config* config, const char* filename)
{
	FILE * file;
	char line [MAX_CONF_ENTRY_LEN];
	char * tok;

	if (!config)
		return 0;

	file = fopen (filename, "r");
	if (!file)
		return 0;

	while (fgets(line, MAX_CONF_ENTRY_LEN, file)) {
		int len = strlen (line);
		char key [MAX_CONF_KEY_LEN];

		if (len <= 1 || line[0] == '#')
			continue;

		if (line[len - 1] == '\n')
			line [len - 1] = '\0';

		tok = strchr (line, '=');
		if (!tok || (tok - line > MAX_CONF_KEY_LEN))
			continue;
		++tok;

		snprintf (key, tok - line, "%s", line);
		if (!log_config_get (config, key))
			log_config_push (config, key, tok);
	}

	fclose (file);
	return 1;
}

void log_config_free (struct log_config* config)
{
	struct log_conf_entry * current = config->begin, * prev;

	while (current) {
		prev = current;
		current = current->next;
		free (prev);
	}
}

int log_config_write (struct log_config* config, char const * filename)
{
	FILE * file;
	int r;
	struct log_conf_entry * e;

	if (!config)
		return 0;

	file = fopen (filename, "w");
	if (!file)
		return 0;

	e = config->begin;

	while (e) {
		r = fprintf (file, "%s=%s\n", e->key, e->value);
		if (r < 0)
			return 0;

		e = e->next;
	}

	fclose (file);
	return 1;
}

void log_config_print_out (struct log_config* config)
{
	struct log_conf_entry* e = config->begin;
	while (e) {
		printf ("[%s] = %s\n", e->key, e->value);
		e = e->next;
	}
}

int log_config_print_key (struct log_config* config, const char* key)
{
	struct log_conf_entry* e = config->begin;
	while (e) {
		if (!strcmp(key, e->key)) {
			printf ("%s\n", e->value);
			return 1;
		}
		e = e->next;
	}
	return 0;
}

void log_config_push (struct log_config* config, const char* key, const char* value)
{
	struct log_conf_entry* e = calloc (1, sizeof(struct log_conf_entry));
	snprintf(e->key, MAX_CONF_KEY_LEN, "%s", key);
	snprintf(e->value, MAX_CONF_VAL_LEN, "%s", value);

	if (!config->last) {
		config->begin = config->last = e;
		return;
	}

	config->last->next = e;
	config->last = e;
}

int log_config_remove (struct log_config* config, const char* key)
{
	struct log_conf_entry* prev;
	struct log_conf_entry* e = config->begin;
	prev = NULL;
	while (e) {
		if (!strcmp(key, e->key)) {
			if (prev)
				prev->next = e->next;
			else
				config->begin = e->next;

			if (e == config->last)
				config->last = prev;

			free (e);
			return 1;
		}
		prev = e;
		e = e->next;

	}
	return 0;
}

int log_config_foreach (struct log_config* config, int (*func)(const char* key, const char* value))
{
	int r = -1;
	struct log_conf_entry* e = config->begin;
	while (e)
	{
		if (!(r = func(e->key, e->value)))
			break;

		e = e->next;
	}
	return r;
}
