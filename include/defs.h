#ifndef ANVIL_DEFINITIONS_H
#define ANVIL_DEFINITIONS_H

/*
 * The purpose of this file is to allow for common definitions.
 * Please include this before any headers.
 */

#define _POSIX_C_SOURCE 200809L

// real mode = ~umask & MKDIR_MODE & 0777
#define MKDIR_MODE 0777

// colors
#define DEFAULT_COLOR "\e[0m"
#define BLACK  "\e[0;30m"
#define RED    "\e[1;31m"
#define GREEN  "\e[1;32m"
#define YELLOW "\e[0;33m"
#define BLUE   "\e[0.34m"
#define CYAN   "\e[0;36m"
#define PURPLE "\e[0;35m"
#define WHITE  "\e[0;37m"

#endif // ANVIL_DEFINITIONS_H
