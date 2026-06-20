#ifndef SHELL_H
#define SHELL_H

#define MAX_INPUT 128
#define MAX_ARG 16

void shell_start(void);
void shell_execute(char* input);

int tokenize(char* input, char* argv[]);

#endif
