#include "defs.h"
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

static int timecmp(struct stat *st1, struct stat *st2)
	__attribute__((nonnull));

int filetime_cmp(const char *restrict file1, const char *restrict file2)
{
	static struct stat file1_st;
	struct stat file2_st;

	if (file1 && (stat(file1, &file1_st) == -1))
		return -1;
	if (stat(file2, &file2_st) == -1)
		return -1;

	return (timecmp(&file1_st, &file2_st)) > 0;
}

static int timecmp(struct stat *st1, struct stat *st2)
{
	return (st1->st_mtim.tv_sec != st2->st_mtim.tv_sec ?
		st1->st_mtim.tv_sec - st2->st_mtim.tv_sec :
		st1->st_mtim.tv_nsec - st2->st_mtim.tv_nsec);
}
