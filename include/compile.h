#ifndef ANVIL_COMPILE_H
#define ANVIL_COMPILE_H

#include "tomlc17.h"

// Maximum size of a command that may be executed by compile().
#define COMPILE_CMD_MAX 4096

int solve_dependencies(const char *target, toml_datum_t toptab, toml_datum_t deps)
    __attribute__((nonnull));

int compile(const char *target, toml_datum_t toptab)
    __attribute__((nonnull));

#endif // ANVIL_COMPILE_H
