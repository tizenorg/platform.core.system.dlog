

if SD_JOURNAL_SUPPORT
AM_CFLAGS = @CFLAGS@ -I$(srcdir)/include \
	      -I$(srcdir)/include/internal -DSD_JOURNAL_SUPPORT
else
AM_CFLAGS = @CFLAGS@ -I$(srcdir)/include \
	      -I$(srcdir)/include/internal
endif

dlog_includedir = $(includedir)/dlog
dlog_include_HEADERS = \
	include/dlog.h

lib_LTLIBRARIES = libdlog.la

libdlog_la_SOURCES =  \
	log.c \
	include/dlog.h

if SD_JOURNAL_SUPPORT
libdlog_la_LIBADD = -lpthread -lsystemd-journal
else
libdlog_la_LIBADD = -lpthread
endif
bin_PROGRAMS= dlogutil

dlogutil_SOURCES = \
	logutil.c \
	logprint.c \
	include/logger.h \
	include/logprint.h

# conf file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlog.pc

bootscriptdir = /etc/rc.d/init.d
bootscript_SCRIPTS = dlog.sh


