void print_help ()
{
	printf ("dlogctrl - provides control over dlog configuration. Options:\n"
		"\t-p        Precludes -kh. Prints all entries in the config.\n"
		"\t-k key    Precludes -ph and requires one of -gs.  Specifies a config key.\n"
		"\t-g        Requires -k, precludes -cs. Gets and prints the value of the entry assigned to the key.\n"
		"\t-s value  Requires -k, precludes -cg. Sets the value of the entry associated with the key.\n"
		"\t-c        Requires -k, precludes -gs. Clears the entry.\n"
		"\t-i        Precludes -pk. Shows information for important config values.\n"
		"Having either -i, -p or -k is mandatory.\n"
	);
}

void print_extended_help ()
{
	printf ("Important entries:\n"
		"\tplog: enable platform logging, ie. whether any logging happens at all. Values are 0 or 1.\n"
		"\tlimiter: enable log limiting. Values are 0 or 1.\n\n"
		"Limiter rules:\n"
		"\tlimiter entry keys are of the form \"limiter|TAG|LEVEL\",\n"
		"\twhere TAG is the arbitrary string which defines the user application,\n"
		"\tand LEVEL is logging level which is one of:\n"
		"\t\tV or 1: verbose\n"
		"\t\tD or 2: debug\n"
		"\t\tI or 3: info\n"
		"\t\tW or 4: warning\n"
		"\t\tE or 5: error\n"
		"\t\tF or 6: fatal\n"
		"\tA rule fits messages of given tag and level. Alternatively,\n"
		"\tan asterisk (*) can also be used in place of either, as a wild card.\n"
		"\tExplicit rules have priority over wild-card ones.\n"
		"\tTwo asterisks can be used to signify a backup rule for those messages\n"
		"\twhich did not fit any other rule. Some example rule keys:\n"
		"\tExample rules:\n"
		"\t\tlimiter|SOME_APP|W\n"
		"\t\tlimiter|SOME_APP|* (affects all SOME_APP logs except warnings, which already have an explicit rule)\n"
		"\t\tlimiter|*|W (affects all warnings except the ones from SOME_APP)\n"
		"\t\tlimiter|*|* (affects everything else)\n"
		"\tLimiter entries can have one of three value types:\n"
		"\t\tstring \"allow\" or integers < 0: lets all logs of given type through.\n"
		"\t\tstring \"deny\" or integers > 10000: blocks all logs of given type.\n"
		"\t\tintegers from the <1, 10000> range: let through that many logs per minute.\n\n"
#ifndef DLOG_BACKEND_JOURNAL
		"Buffer size entries:\n"
		"\tkeys are of the form <BUFFER>_size, for example main_size;\n"
		"\tvalues are size in bytes, for example 1048576.\n"
		"\tThese entries control the size of the logging buffers.\n\n"
		"Passive logging to file (dlog_logger):\n"
		"\tkeys are of the format dlog_logger_conf_#\n"
		"\twhere # are consecutive integers from 0.\n"
		"\tValues are the same as when calling dlogutil (including \"dlogutil\" at start), for example\n"
		"\t\tdlogutil -b system -r 5120 -n 1 -f /var/log/dlog/system.raw *:I\n\n"
#ifndef DLOG_BACKEND_PIPE
		"Buffer device names:\n"
		"\tkeys are simply buffer names, for example main;\n"
		"\tvalues are paths to the devices.\n"
		"\tNote: these values are read-only!\n"
		"\tChanging them does nothing and can potentially break things.\n\n"
#endif
#endif
#ifdef DLOG_BACKEND_PIPE
		"DLogUtil sorting:\n"
		"\tkey is util_sorting_time_window;\n"
		"\tvalue is an integer representing the size\n"
		"\tof the sorting window in milliseconds. This\n"
		"\taffects quality of the sort, but also delay.\n\n"
		"Socket control entries:\n"
		"\tkeys are of the form <BUFFER>_{ctl,write}_sock_{path,rights,group,owner}\n"
		"\tFor example: system_write_sock_group, radio_ctl_sock_path\n"
		"\tCTL sockets are for administrative purposes, WRITE are for programs making logs.\n"
		"\tFor PATH, values signify the path to the appropriate socket.\n"
		"\tFor RIGHTS, the value is file permissions in octal (for example 0664).\n"
		"\tFor GROUP and OWNER the values are group/owner of the file (for example root).\n\n"
#endif
	);
}
