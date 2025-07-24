#include "defs.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "filetime.h"
#include "tomlc17.h"

void print_main_help();

i32 cmd_init(int argc, char **argv);
i32 cmd_build(int argc, char **argv);
i32 cmd_help(int argc, char **argv);

typedef i32 (*CommandFn)(i32 argc, char **argv);

typedef struct {
    const char *name;
    const char *help;
    CommandFn fn;

} Command;

Command commands[] = {
    {"init", "Initialize the project", cmd_init},
    {"build", "Build the project", cmd_build},
    {"help", "Show help message", cmd_help},
    {NULL, NULL, NULL} // sentinel
};

i32 main(i32 argc, char **argv) {
    filetime_cmp("", "");

    if (argc <= 1) {
        fprintf(stderr, "Usage: %s <command> [options...]\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    for (i32 i = 0; commands[i].name != NULL; i++) {
        if (!strcmp(cmd, commands[i].name)) {
            return commands[i].fn(argc - 1, argv + 1);
        }
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    return 1;
}

int cmd_help(int argc, char **argv) {
    printf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++)
        printf("  %-10s %s\n", commands[i].name, commands[i].help);

    return 0;
}

i32 cmd_init(int argc, char **argv) {
    assert(argc == 1);
    char *project_name = argv[1];

    struct stat st;
    bool exists = stat(project_name, &st);
    bool is_dir = S_ISDIR(st.st_mode);

    // TODO
}

i32 cmd_build(int argc, char **argv) {
    // TODO
}
