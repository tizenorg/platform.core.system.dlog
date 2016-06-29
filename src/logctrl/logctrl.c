#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <logconfig.h>
#include "logctrl_doc.h"

struct options {
	int print_all;
	int has_key;
	int help;
	int should_get;
	int should_set;
	int should_clear;
};

int validate(struct options o)
{
	int valid = 1;

	if (!o.has_key && !o.print_all && !o.help) {
		printf ("Having either -i, -p, or -k is mandatory!\n");
		valid = 0;
	}
	if (o.has_key && !(o.should_get || o.should_set || o.should_clear)) {
		printf("-k requires either -c, -g, or -s!\n");
		valid = 0;
	}
	if ((o.has_key + o.print_all + o.help) > 1) {
		printf ("-i, -p and -k preclude each other!\n");
		valid = 0;
	}
	if (o.should_set + o.should_get + o.should_clear > 1) {
		printf("-c, -g and -s preclude each other!\n");
		valid = 0;
	}
	if (!o.has_key && (o.should_set || o.should_get || o.should_clear)) {
		printf("-c, -g and -s require -k!\n");
		valid = 0;
	}

	return valid;
}

int main(int argc, char ** argv)
{
	struct log_config conf;
	char key[MAX_CONF_KEY_LEN];
	char val[MAX_CONF_VAL_LEN];
	struct options opt = {0, 0, 0, 0, 0, 0};
	const char *filename = getenv("DLOG_CONFIG_PATH") ? : get_config_filename(CONFIG_TYPE_COMMON);

	if (argc == 1) {
		print_help(argv[0]);
		return 0;
	}

	for (;;) {
		int ret = getopt(argc, argv, "pt:k:gs:ci");

		if (ret < 0)
			break;

		switch (ret) {
		case 'p':
			opt.print_all = 1;
			break;
		case 'k':
			snprintf(key, MAX_CONF_KEY_LEN, "%s", optarg);
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
		case 'i':
			opt.help = 1;
			break;
		}
	}

	if (!validate(opt)) {
		print_help(argv[0]);
		return 1;
	}

	if (opt.help) {
		print_extended_help();
		return 1;
	}

	if (!log_config_read(&conf)) {
		printf("Error: cannot open the config file!\n");
		return 1;
	}

	if (opt.print_all)
		log_config_print_out(&conf);
	else if (opt.should_get) {
		if (!log_config_print_key(&conf, key))
			goto err_entry;
	} else if (opt.should_clear) {
		if (!log_config_remove(&conf, key))
			goto err_entry;
		if (!log_config_write(&conf, filename))
			goto err_save;
	} else if (opt.should_set) {
		if (!log_config_set(&conf, key, val))
			log_config_push(&conf, key, val);
		if (!log_config_write(&conf, filename))
			goto err_save;
	} else {
		printf("Error: invalid options\n");
		return 1;
	}

	return 0;

err_save:
	printf("Cannot save file\n");
	return 1;

err_entry:
	printf("Entry not found\n");
	return 1;
}
