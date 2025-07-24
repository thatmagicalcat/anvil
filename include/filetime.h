#ifndef ANVIL_FILETIMES_H
#define ANVIL_FILETIMES_H

/*
 * Works like strcmp() but with file access times. If file1 is NULL, it
 * is assumed to be the same as from the last call (this calls one less syscall).
 *
 * If operation fails, -1 is returned and errno is set.
 * If everything succeeds, 0 means file1 was accessed before or at the same time
 * as file2, if it returns 1, it means file1 was accessed after file2
 */
int filetime_cmp(const char *restrict file1, const char *restrict file2)
	__attribute__((nonnull (2)));

#endif /* ANVIL_FILETIMES_H */
