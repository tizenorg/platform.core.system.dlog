#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <logconfig.h>

void print_help ()
{
	printf ("dlogctrl - provides control over dlog configuration. Options:\n"
		"\t-p        Precludes -k. Prints all entries in the config.\n"
		"\t-k key    Precludes -p and requires one of -gs.  Specifies a config key.\n"
		"\t-g        Requires -k, precludes -cs. Gets and prints the value of the entry assigned to the key.\n"
		"\t-s value  Requires -k, precludes -cg. Sets the value of the entry associated with the key.\n"
		"\t-c        Requires -k, precludes -gs. Clears the entry."
		"Having either -p or -k is mandatory.\n"
	);
}

struct options {
	int print_all;
	int has_key;
	int should_get;
	int should_set;
	int should_clear;
};

int validate (struct options o) {
	int valid = 1;

	if (!o.has_key && !o.print_all) {
		printf ("Having either -p or -k is mandatory!\n");
		valid = 0;
	}
	if (o.has_key && !(o.should_get || o.should_set || o.should_clear)) {
		printf ("-k requires either -c, -g, or -s!\n");
		valid = 0;
	}
	if (o.has_key && o.print_all) {
		printf ("-p and -k preclude each other!\n");
		valid = 0;
	}
	if (o.should_set + o.should_get + o.should_clear > 1) {
		printf ("-c, -g and -s preclude each other!");
		valid = 0;
	}
	if (!o.has_key && (o.should_set || o.should_get || o.should_clear)) {
		printf ("-c, -g and -s require -k!\n");
		valid = 0;
	}

	return valid;
}

int main (int argc, char ** argv)
{
	struct log_config conf;
	char key [MAX_CONF_KEY_LEN];
	char val [MAX_CONF_VAL_LEN];
	struct options opt = {0,0,0,0,0};
	const char * filename = getenv ("DLOG_CONFIG_KMSG_PATH") ?: get_config_filename (CONFIG_TYPE_COMMON);

	if (argc == 1) {
		print_help (argv[0]);
		return 0;
	}

	for (;;) {
		int ret = getopt(argc, argv, "pt:k:gs:c");

		if (ret < 0)
			break;

		switch (ret) {
		case 'p':
			opt.print_all = 1;
			break;
		case 'k':
			snprintf (key, MAX_CONF_KEY_LEN, "%s", optarg);
			opt.has_key = 1;
			break;
		case 'g':
			opt.should_get = 1;
			break;
		case 'c':
			opt.should_clear = 1;
			break;
		case 's':
			opt.should_set = 1;
			snprintf(val, MAX_CONF_VAL_LEN, "%s", optarg);
			break;
		}
	}

	if (!validate(opt)) {
		print_help (argv[0]);
		return 1;
	}

	if (!log_config_read (&conf)) {
		printf ("Error: cannot open the config file!\n");
		return 1;
	}

	if (opt.print_all) {
		log_config_print_out (&conf);
	} else if (opt.should_get) {
		if (!log_config_print_key (&conf, key))
			goto err_entry;
	} else if (opt.should_clear) {
		if (!log_config_remove(&conf, key))
			goto err_entry;
		if (!log_config_write (&conf, filename))
			goto err_save;
	} else if (opt.should_set) {
		if (!log_config_set(&conf, key, val))
			log_config_push(&conf, key, val);
		if (!log_config_write (&conf, filename))
			goto err_save;
	} else {
		printf ("Error: invalid options\n");
		return 1;
	}

	return 0;

err_save:
	printf("Cannot save file\n");
	return 1;

err_entry:
	printf ("Entry not found\n");
	return 1;
}