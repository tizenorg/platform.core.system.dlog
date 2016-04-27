#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <logconfig.h>

int main (int argc, char ** argv)
{
	struct log_config conf;
	struct log_conf_entry * e;

	log_config_read (&conf);

	if (argc == 2 && !strcmp(argv[1], "get")) {
		e = conf.begin;
		while (e) {
			printf ("[%s] = %s\n", e->key, e->value);
			e = e -> next;
		}
		return 0;
	}

	if (argc == 3 && !strcmp(argv[1], "get")) {
		e = conf.begin;
                while (e) {
			if (!strcmp(argv[2], e->key)) {
				printf ("%s\n", e->value);
				return 0;
			}
                        e = e -> next;
		}
		printf ("Entry not found\n");
		return 1;
	}

	if (argc == 4 && !strcmp(argv[1], "set")) {
		e = conf.begin;
		while (e) {
			if (!strcmp(argv[2], e->key)) {
				strncpy (e->value, argv[3], 256);
				log_config_write (&conf);
				return 0;
			}
			e = e -> next;
		}

		e = calloc(1, sizeof(struct log_conf_entry));
		strncpy (e->key, argv[2], 32);
		strncpy (e->value, argv[3], 256);
		e -> next = conf.begin;
		conf.begin = e;
		log_config_write (&conf);
		return 0;
	}

	if (argc == 3 && !strcmp(argv[1], "clear")) {
		struct log_conf_entry * prev;
		e = conf.begin;
		prev = NULL;
		while (e) {
			if (!strcmp(argv[2], e->key)) {
				if (prev)
					prev->next = e->next;
				else
					conf.begin = e->next;
				log_config_write (&conf);
				return 0;
			}
			prev = e;
			e = e->next;
		}
		printf ("Entry not found\n");
		return 1;
	}

	printf("Usage:\n"
		"\tdlogctrl get - print values of all config entries\n"
		"\tdlogctrl get (key) - prints the value of the config entry listed under (key)\n"
		"\tdlogctrl set (key) (value) - sets the config entry under (key) to (value)\n"
		"\tdlogctrl clear (key) - remove the config entry under (key)\n"
	);

	return 1;
}
