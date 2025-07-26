#include "defs.h"
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "filetime.h"
#include "tomlc17.h"
#include "compile.h"

// Helper function. Returns flags used to compile the target and sets their size o sz
static const char *get_flags(const char *target, toml_datum_t targtab, size_t *sz)
    __attribute__((nonnull));

// Helper function. Returns full compiling command. NULL if it fails
static const char *get_compile_cmd(const char *targett, toml_datum_t targtab)
    __attribute__((nonnull));

// Helper function. Removes extension from a file path
static int remove_extension(char *fname) __attribute__((nonnull));

static const char *get_extension(const char *fname) __attribute__((nonnull));

/*
 * Compiles a file and it's dependencies as objects.
 * TODO: Check if an object file alredy exists and if
 *       it is newer or older then it's dependencies.
 *       (This currently may compile the same target
 *       more then once.)
 */
int compile(const char *target, toml_datum_t toptab)
{
    toml_datum_t targtab;
    toml_datum_t deps;
    char *target_name;

    /* Allocates target_name and removes it's extension. It is dynamically
     * Allocated to aliviate the stack call as solve_dependencies() calls
     * compile() (It is freed before any recursion happens)
     */
    target_name = malloc((PATH_MAX + 1) * sizeof(*target_name));
    if (!target_name) {
        return -1;
    }
    strncpy(target_name, target, PATH_MAX);
    if (remove_extension(target_name) == -1) {
        fprintf(stderr, "could not remove extension from %s\n", target_name);
        free(target_name);
        return -1;
    }

    // gets the target toml table
    targtab = toml_seek(toptab, target_name);
    free(target_name);
    if (targtab.type != TOML_TABLE) {
        if (targtab.type == TOML_UNKNOWN) {
            return compile_no_config(target);
        }
        fprintf(stderr, "%s table is either of invalid type or does not exists\n", target);
        return -1;
    }

    // gets the targer dependencies toml table
    deps = toml_seek(targtab, "dependencies");
    if (deps.type != TOML_ARRAY) {
        // This allows for no dependencies
        if (deps.type == TOML_UNKNOWN) {
            goto compile_target;
        }
        fprintf(stderr, "%s.dependencies is either of invalid type"
                        "or does not exist\n", target);
        return -1;
    }

    // Solves all dependencies from a file. returns -1 if operation fails
    if (solve_dependencies(target, toptab, deps) == -1) {
        fprintf(stderr, "could not solve dependencies for %s\n", target);
        return -1;
    }

compile_target:
    // Writes command on the screen and executes it if it is not NULL
    const char *cmd = get_compile_cmd(target, targtab);
    if (!cmd || (puts(cmd) && system(cmd) == -1)) {
        return -1;
    }

    return 0;
}

int link_all(toml_datum_t toptab)
{
    toml_datum_t tmptab;
    DIR *dirp;
    struct dirent *dep;
    const char *ext;
    char cmd[LINK_CMD_MAX];

    strncpy(cmd, "ccache gcc ", LINK_CMD_MAX);

    dirp = opendir(".");
    if (!dirp) {
        perror("link_all");
        return -1;
    }

    while ((dep = readdir(dirp))) {
        ext = get_extension(dep->d_name);
        if (!ext || (strcmp(ext, ".o") != 0))  {
            continue;
        } else if (strnlen(cmd, LINK_CMD_MAX) + _D_EXACT_NAMLEN(dep) + 2 > LINK_CMD_MAX) {
            fputs("Linking command is too big. You can try recompiling anvil after"
                    "Changing LINK_CMD_MAX at include/compile.h\n", stderr);
            return -1;
        }

        strcat(cmd, dep->d_name);
        strcat(cmd, " ");
    }

    tmptab = toml_seek(toptab, "package.name");
    if (tmptab.type != TOML_STRING) {
        fputs("You must have a string value for package.name at your config file\n", stderr);
        return -1;
    }

    if (strnlen(cmd, LINK_CMD_MAX) + (size_t)tmptab.u.str.len + 4 > LINK_CMD_MAX) {
            fputs("Linking command is too big. You can try recompiling anvil after"
                    "Changing LINK_CMD_MAX at include/compile.h\n", stderr);
            return -1;
    }

    strcat(cmd, "-o ");
    strncat(cmd, tmptab.u.str.ptr, (size_t)tmptab.u.str.len);
    puts(cmd);
    return system(cmd);
}

/*
 * Solves all the dependencies of a target (recursevely calls compile() on them).
 * TODO: implement a recursion checker to prevent against circular dependencies
 */
int solve_dependencies(const char *target, toml_datum_t toptab, toml_datum_t deps)
{
    int ret;

    // Iterates trough all dependencies and calls compile() on them.
    for (int i = 0; i < deps.u.arr.size; ++i) {
        // error checks
        if (deps.u.arr.elem[i].type != TOML_STRING) {
            fprintf(stderr, "dependency number %i of %s is of invalid type\n", i, target);
            return -1;
        // checks if solving depedency is needed. If check fails, this is executed
        } else if ((ret = filetime_cmp(target, deps.u.arr.elem[i].u.str.ptr)) == -1) {
            fprintf(stderr, "%s: %s\n", deps.u.arr.elem[i].u.str.ptr, strerror(errno));
            return -1;
        // if dependency has to be solved, attempts to solves it.
        } else if (ret) {
            if (compile(deps.u.arr.elem[i].u.str.ptr, toptab) == -1) {
                return -1;
            }
        }
    }

    return 0;
}

int compile_no_config(const char *target)
{
    char cmd[COMPILE_CMD_MAX] = "ccache gcc -c ";
    if (strnlen(cmd, COMPILE_CMD_MAX) + strlen(target) + 1 > COMPILE_CMD_MAX) {
        errno = ERANGE;
        return -1;
    }
    strcat(cmd, target);
    puts(cmd);
    return system(cmd);

}


// Helper functions:
static const char *get_compile_cmd(const char *target, toml_datum_t targtab)
{
    size_t flags_sz;
    static char cmd[COMPILE_CMD_MAX];

    // Checks if a shell exists
    if (!system(NULL)) {
        fprintf(stderr, "no shell available while processing target %s\n", target);
        return NULL;
    }

    // Gets flags for the compile command
    const char *flags = get_flags(target, targtab, &flags_sz);
    if (!flags) {
        return NULL;
    }

    // Builds the compile command and returns it. Returns NULL if it is too big
    strcpy(cmd, "ccache gcc -c ");
    if (strnlen(cmd, COMPILE_CMD_MAX) + strlen(target) + flags_sz + 1 > COMPILE_CMD_MAX) {
        fprintf(stderr, "flags argument for target %s is too long\n", target);
        errno = ERANGE;
        return NULL;
    }

    strcat(cmd, target);
    strcat(cmd, " ");
    strncat(cmd, flags, flags_sz);
    return cmd;
}

static const char *get_flags(const char *target, toml_datum_t targtab, size_t *sz)
{
    // Gets the flags string on the targets tab
    toml_datum_t flags_tab = toml_seek(targtab, "flags");
    if (flags_tab.type != TOML_STRING) {
        // If there are no flags, exit early and return an empty string
        if (flags_tab.type == TOML_UNKNOWN) {
            *sz = 0;
            return "";
        }

        fprintf(stderr, "flags argument of %s is of invalid type\n", target);
        errno = EINVAL;
        return NULL;
    }

    assert(flags_tab.u.str.len >= 0);
    *sz = (size_t)flags_tab.u.str.len;
    return flags_tab.u.str.ptr;
}

static int remove_extension(char *fname)
{
    const size_t str_size = strlen(fname);
    for (size_t i = str_size-1; i < str_size; --i) {
        if (fname[i] == '.') {
            fname[i] = '\0';
            return 0;
        }
    }

    return -1;
}

static const char *get_extension(const char *fname)
{
    static char ext[PATH_MAX+1];
    size_t i, j, n, k;

    n = strlen(fname);
    assert(n <= PATH_MAX);

    for (i = n-1, k = 0; i <= n && i <= PATH_MAX; --i, ++k) {
        if (fname[i] == '.') {
            break;
        }
    }

    for (i = n-k-1, j = 0; i < n && j < PATH_MAX; ++i, ++j) {
        ext[j] = fname[i];
    }

    ext[j] = '\0';
    return ext;
}
