#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <logconfig.h>

int main (int argc, char ** argv)
{
	struct log_config conf;

	if (!log_config_read (&conf, CONFIG_FILENAME)) {
		printf ("Cannot open config file\n");
		return 1;
	} else if (argc == 2 && !strcmp(argv[1], "get")) {
		log_config_print_out (&conf);
	} else if (argc == 3 && !strcmp(argv[1], "get")) {
		if (!log_config_print_key (&conf, argv[2]))
		    goto err_entry;
	} else if (argc == 4 && !strcmp(argv[1], "set")) {
		if (!log_config_set(&conf, argv[2], argv[3]))
			log_config_push(&conf, argv[2], argv[3]);

		if (!log_config_write (&conf, CONFIG_FILENAME))
			goto err_save;
	} else if (argc == 3 && !strcmp(argv[1], "clear")) {
		if (!log_config_remove(&conf, argv[2]))
			goto err_entry;

		if (!log_config_write (&conf, CONFIG_FILENAME))
			goto err_save;
	} else {
		printf("Usage:\n"
			"\tdlogctrl get - print values of all config entries\n"
			"\tdlogctrl get (key) - prints the value of the config entry listed under (key)\n"
			"\tdlogctrl set (key) (value) - sets the config entry under (key) to (value)\n"
			"\tdlogctrl clear (key) - remove the config entry under (key)\n");

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
