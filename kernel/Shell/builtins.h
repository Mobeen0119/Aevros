#ifndef BUILTINS_H
#define BUILTINS_H

#include "../../Include/vfs.h"

void cmd_cd(char *path);

void cmd_ls(void);

void cmd_cat(char *path);

void cmd_echo(char *text);

void cmd_touch(char *path);

void cmd_write(const char *path, char *txt);

void cmd_rm(char *path);

void print_path(dentry_t *dir);

void cmd_pwd(void);

void tree_walk(dentry_t *dir, int depth);

#endif