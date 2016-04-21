#include <logconfig.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const char * log_config_get (struct log_config* c, const char* key)
{
	struct log_conf_entry * e;

	if (!c) return NULL;
	e = c->begin;

	while (e) {
		if (!strcmp (e->key, key)) return e->value;
		e = e->next;
	}

	return NULL;
}

void log_config_set (struct log_config* c, const char * key, const char * value)
{
	struct log_conf_entry * e;

	if (!c || !key || !value) return;
	e = c->begin;

	while (e) {
		if (!strcmp (e->key, key)) {
			strncpy (e->value, value, 256);
			return;
		}
		e = e->next;
	}
}

int log_config_read (struct log_config* config)
{
	FILE * file;
	char line [512];
	char * tok;

	if (!config) return 0;

	file = fopen (CONFIG_FILENAME, "r");
	if (!file) return 0;

	config->begin = NULL;

	while (fgets(line, 512, file)) {
		int len = strlen (line);
		struct log_conf_entry * e;

		if (len <= 1) continue;
		if (line[0] == '#') continue;

		e = calloc (1, sizeof(struct log_conf_entry));

		tok = strtok (line, "=");
		strncpy (e->key, tok, 32);
		tok = strtok (NULL, "\n");
		strncpy (e->value, tok, 256);

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

	if (!config) return 0;

	file = fopen (CONFIG_FILENAME, "w");
	if (!file) return 0;

	e = config->begin;

	while (e) {
		fprintf (file, "%s=%s\n", e->key, e->value);
		e = e->next;
	}

	fclose (file);
	return 1;
}
