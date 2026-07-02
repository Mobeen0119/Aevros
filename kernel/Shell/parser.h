#include "../../Include/shell.h"


static int strcmp_cmd(char *a, char *b);

static void parse(char *input, char **cmd, char **arg);

int tokenize(char *input, char *argv[]);