#include <stdint.h>
#include "../../Lib/string.c"
#include "../../Include/vfs.h"
#include "../Process/task.h"

#define MAX_CMD 128

static char cmd_buffer[MAX_CMD];
