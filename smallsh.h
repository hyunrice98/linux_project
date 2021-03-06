#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define EOL 1
#define ARG 2
#define AMPERSAND 3
#define SEMICOLON 4

#define MAXARG 512
#define MAXBUF 512

#define FOREGROUND 0
#define BACKGROUND 1

int userin();

void procline();

int gettok(char **outptr);

int inarg(char c);

int runcommand(char **cline, int where);

void chDir(char **arg, int narg);

int fileOutput(char **cline);

int pipeHandler(char **argv1, char **argv2);