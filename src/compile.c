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

int compile(const char *target, toml_datum_t toptab)
{
    int ret;
    toml_datum_t table = toml_seek(toptab, target);
    if (table.type != TOML_TABLE) {
        fprintf(stderr, "%s table is either of invalid type or does not exists\n", target);
        errno = EINVAL;
        return -1;
    }

    toml_datum_t deps = toml_seek(table, "dependencies");
    if (deps.type != TOML_ARRAY) {
        fprintf(stderr, "%s.dependencies is either of invalid type"
                        "or does not exist\n", target);
        errno = EINVAL;
        return -1;
    }

    if (solve_dependencies(target, toptab, deps) == -1) {
        int save_errno = errno;
        fprintf(stderr, "could not solve dependencies for %s\n", target);
        errno = save_errno;
        return -1;
    }

    return 0;
}

int solve_dependencies(const char *target, toml_datum_t toptab, toml_datum_t deps)
{
    i32 ret;

    for (i32 i = 0; i < deps.u.arr.size; ++i) {
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
