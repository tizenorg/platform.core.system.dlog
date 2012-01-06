#include <tet_api.h>
#include "dlog.h"
#define LOG_BUF_SIZE 1024
static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW___dlog_print_func_01(void);
static void utc_ApplicationFW___dlog_print_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW___dlog_print_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW___dlog_print_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 }
};

//static int pid;

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of __dlog_print()
 */
static void utc_ApplicationFW___dlog_print_func_01(void)
{
	int r = 0;
	
   	r = __dlog_print(LOG_ID_MAIN, DLOG_DEBUG,"DLOG_TEST", "dlog test message for tetware\n");

	if (r<0) {
		tet_printf("__dlog_print() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init __dlog_print()
 */
static void utc_ApplicationFW___dlog_print_func_02(void)
{
	int r = 0;

   	r = __dlog_print( LOG_ID_MAX , DLOG_DEBUG,"DLOG_TEST", "dlog test message for tetware\n");

	if (r>=0) {
		tet_printf("__dlog_print() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}
