#include <logconfig.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

void log_config_set (struct log_config* c, const char * key, const char * value)
{
	struct log_conf_entry * e;

	if (!c || !key || !value)
		return;

	e = c->begin;
	while (e) {
		if (!strcmp (e->key, key)) {
			strncpy (e->value, value, MAX_CONF_VAL_LEN);
			return;
		}
		e = e->next;
	}
}

int log_config_read (struct log_config* config)
{
	FILE * file;
	char line [MAX_CONF_ENTRY_LEN];
	char * tok;

	if (!config)
		return 0;

	file = fopen (CONFIG_FILENAME, "r");
	if (!file)
		return 0;

	config->begin = NULL;

	while (fgets(line, MAX_CONF_ENTRY_LEN, file)) {
		int len = strlen (line);
		struct log_conf_entry * e;

		if (len <= 1 || line[0] == '#')
			continue;

		if (line[len - 1] == '\n')
			line [len - 1] = '\0';

		tok = strchr (line, '=');
		if (!tok || (tok - line > MAX_CONF_KEY_LEN)) {
			fprintf (stderr, "Invalid dlog config line: %s\n", line);
			continue;
		}
		++tok;

		e = calloc (1, sizeof(struct log_conf_entry));
		snprintf(e->key, tok - line, "%s", line);
		snprintf(e->value, MAX_CONF_VAL_LEN, "%s", tok);

		e->next = config->begin;
		config->begin = e;
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

int log_config_write (struct log_config* config)
{
	FILE * file;
	struct log_conf_entry * e;

	if (!config)
		return 0;

	file = fopen (CONFIG_FILENAME, "w");
	if (!file)
		return 0;

	e = config->begin;

	while (e) {
		fprintf (file, "%s=%s\n", e->key, e->value);
		e = e->next;
	}

	fclose (file);
	return 1;
}
