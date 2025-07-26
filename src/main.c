#include "defs.h"
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef POSIXLY_CORRECT
# undef POSIXLY_CORRECT
#endif
#include <getopt.h>

static int init_dirs(size_t n, const char *paths[n])
    __attribute__((nonnull_if_nonzero(2,1)));
static int init_toml(const char *pkgname) __attribute__((nonnull));

int cmd_init(int argc, char *argv[argc + 1]) __attribute__((nonnull));
int cmd_build(int argc, char *argv[argc + 1]) __attribute__((nonnull));
int cmd_clean(int argc, char *argv[argc + 1]) __attribute__((nonnull));


struct Command {
    int (*fn)(int argc, char **argv);
    const char *name;
    const char *help;
};

static struct Command commands[] = {
    {.fn=cmd_init, .name="init", .help="Initializes a project"},
    {.fn=cmd_build, .name="build", .help="Compiles all files on a project into a single one"},
    {.fn=cmd_clean, .name="clean", .help="Cleans object files"},
    {0} // sentinel
};

int main(int argc, char *argv[argc + 1])
{
    size_t i;
    int ret = -1;
    const char *cmd = argv[1];
    for (i = 0; commands[i].name; ++i) {
        if (strcmp(cmd, commands[i].name) == 0) {
            ret = commands[i].fn(argc - 1, argv + 1);
        }
    }

    if (ret == -1) {
        if (commands[i].help) { 
            fputs(commands[i].help, stderr);
        } else {
            fprintf(stderr, "\"%s\" is not an existing command\n", cmd);
        }
    }

    return ret;
}


int cmd_init(int argc, char *argv[argc + 1])
{
    static const size_t n = 3;
    const char *init_dirpaths[] = {
        "src", "include", "build",
    };

    assert(argc >= 2);
    DIR *dirp;

    errno = 0;
    const char *pkgname = argv[1];
    // Opens directory, if it does not exists, creates it.
    if (!(dirp = opendir(pkgname))) {
        if (errno != ENOENT) {
            goto init_err;
        }
        errno = 0;
        if (mkdir(pkgname, MKDIR_MODE) == -1) {
            goto init_err;
        }
        errno = 0;
        if (!(dirp = opendir(pkgname))) {
            goto init_err;
        }
    }
    //  Entes the directory
    if (chdir(pkgname) == -1) {
        goto init_err;
    }

    // these functions alredy have their own error msgs, so no init_err.
    if (init_dirs(n, init_dirpaths) == -1 || init_toml(pkgname) == -1) {
        chdir("..");
        return -1;
    }

    // if success
    return chdir("..");
init_err:
    perror(argv[0]);
    chdir("..");
    return -1;
}

static int init_dirs(size_t n, const char *paths[n])
{
    // iterates trough all paths and creates directories
    for (size_t i = 0; i < n; ++i) {
        errno = 0;
        if (mkdir(paths[i], MKDIR_MODE) == -1)  {
            if (errno == EEXIST) {
                fprintf(stderr, "%s[-]%s %s alredy exists\n",
                        YELLOW, DEFAULT_COLOR, paths[i]);
                fflush(stderr); // flushes to allows color to screen
            } else {
                fprintf(stderr, "%s[x]%s while creating %s: %s\n",
                        RED, DEFAULT_COLOR, paths[i], strerror(errno));
                fflush(stderr); // flushes to allows color to screen
                return -1;
            }
        } else {
            printf("%s[v]%s created %s\n", GREEN, DEFAULT_COLOR, paths[i]);
            fflush(stdout); // flushes to allows colors to screen
        }
    }

    return 0;
}

static int init_toml(const char *pkgname)
{
    static const size_t n = 6;
    const char* fmt[] = {
        "[%s]",
        "name=%s",
        "[%s]",
        "include=%s",
        "source=%s",
        "build=%s"
    };
    const char *fmtarg[] = {
        "package",
        pkgname,
        "build",
        "include",
        "src",
        "build"
    };

    errno = 0;
    FILE *fp = fopen("anvil.toml", "w");
    if (!fp) {
        fprintf(stderr, "%s[x]%s while creating anvil.toml: %s\n",
                RED, DEFAULT_COLOR, strerror(errno));
        return -1;
    }

    for (size_t i = 0; i < n; ++i) {
        errno = 0;
        if ((fprintf(fp, fmt[i], fmtarg[i]) < 0) || (fputc('\n', fp) == EOF)) {
            fprintf(stderr, "%s[x]%s while writing to %s\n",
                    RED, DEFAULT_COLOR, pkgname);
            fflush(stderr); // allow color to screen
            return -1;
        }
    }

    return 0;
}

int cmd_build(int argc, char *argv[argc + 1]) { (void)argv; return argc; }
int cmd_clean(int argc, char *argv[argc + 1]) { (void)argv; return argc; }
