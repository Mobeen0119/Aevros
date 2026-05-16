
#define SHELL_H

#define MAX_INPUT 256
#define MAX_ARG 16

void shell_start();
void shell_execute(char* input);

int parse_command(char* input,char* argv[]);
