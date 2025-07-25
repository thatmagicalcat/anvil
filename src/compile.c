#include "defs.h"
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "filetime.h"
#include "tomlc17.h"
#include "compile.h"

// Helper function. Returns flags used to compile the target and sets their size o sz
static const char *get_flags(const char *target, toml_datum_t targtab, int *sz)
    __attribute__((nonnull));
// Helper function. Returns full compiling command. NULL if it fails
static const char *get_compile_cmd(const char *target, toml_datum_t targtab)
    __attribute__((nonnull));


// Compiles target based on the top tab of a toml file
int compile(const char *target, toml_datum_t toptab)
{
    int ret;

    toml_datum_t targtab = toml_seek(toptab, target);
    if (targtab.type != TOML_TABLE) {
        fprintf(stderr, "%s table is either of invalid type or does not exists\n", target);
        errno = EINVAL;
        return -1;
    }

    toml_datum_t deps = toml_seek(targtab, "dependencies");
    if (deps.type != TOML_ARRAY) {
        // This allows for no dependencies
        if (deps.type == TOML_UNKNOWN) {
            goto compile_target;
        }
        fprintf(stderr, "%s.dependencies is either of invalid type"
                        "or does not exist\n", target);
        errno = EINVAL;
        return -1;
    }

    // Solves all dependencies from a file. Aborts if operation fails
    errno = 0;
    if (solve_dependencies(target, toptab, deps) == -1) {
        int save_errno = errno;
        fprintf(stderr, "could not solve dependencies for %s\n", target);
        errno = save_errno;
        return -1;
    }

compile_target:
    const char *cmd = get_compile_cmd(target, targtab);
    if (!cmd) {
        return -1;
    }

    puts(cmd);
    if (system(cmd) == -1) {
        int save_errno = errno;
        fprintf(stderr, "%s: %s\n", cmd, strerror(save_errno));
        errno = save_errno;
        return -1;
    }
    return 0;
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
        if (deps.u.arr.elem[i].type != TOML_STRING) {
            fprintf(stderr, "dependency number %i of %s is of invalid type\n", i, target);
            errno = EINVAL;
            return -1;
        } else if ((ret = filetime_cmp(target, deps.u.arr.elem[i].u.str.ptr)) == -1) {
            int save_errno = errno;
            fprintf(stderr, "%s: %s\n", deps.u.arr.elem[i].u.str.ptr, strerror(save_errno));
            errno = save_errno;
            return -1;
        } else if (ret) {
            if (compile(deps.u.arr.elem[i].u.str.ptr, toptab) == -1) {
                return -1;
            }
        }
    }

    return 0;
}


// Helper functions:

static const char *get_compile_cmd(const char *target, toml_datum_t targtab)
{

    int flags_sz;
    static char cmd[COMPILE_CMD_MAX];

    if (!system(NULL)) {
        fprintf(stderr, "no shell available while processing target %s\n", target);
        return NULL;
    }

    const char *flags = get_flags(target, targtab, &flags_sz);
    if (!flags) {
        return NULL;
    }

    strcpy(cmd, "ccache gcc ");
    if (strnlen(cmd, COMPILE_CMD_MAX) + strlen(target) + flags_sz + 4 > COMPILE_CMD_MAX) {
        fprintf(stderr, "flags argument for target %s is too long\n", target);
        errno = ERANGE;
        return NULL;
    }
    
    strcat(cmd, target);
    strcat(cmd, ".c ");
    strncat(cmd, flags, flags_sz);
    return cmd;
}

static const char *get_flags(const char *target, toml_datum_t targtab, int *sz)
{
    toml_datum_t flags_tab = toml_seek(targtab, "flags");
    if (flags_tab.type != TOML_STRING) {
        // This allows to not set any flags
        if (flags_tab.type == TOML_UNKNOWN) {
            *sz = 0;
            return "";
        }

        fprintf(stderr, "flags argument of %s is of invalid type\n", target);
        errno = EINVAL;
        return NULL;
    }

    *sz = flags_tab.u.str.len;
    return flags_tab.u.str.ptr;
}
