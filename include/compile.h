#ifndef ANVIL_COMPILE_H
#define ANVIL_COMPILE_H

#include "tomlc17.h"

// Maximum size of a command that may be executed by compile().
#define COMPILE_CMD_MAX 4096

// Solves all the dependencies of a file
int solve_dependencies(const char *target, toml_datum_t toptab, toml_datum_t deps)
    __attribute__((nonnull));

/*
 * Compiles a target and it's dependencies. You do not need to call
 * solve_dependencies() before this.
 */
int compile(const char *target, toml_datum_t toptab) __attribute__((nonnull));

/*
 * Compiles a target but completely ignores all of it's configuration.
 * Usefull for debugging and compiling files without configuration
 */
int compile_no_config(const char *target) __attribute__((nonnull));

#endif // ANVIL_COMPILE_H
