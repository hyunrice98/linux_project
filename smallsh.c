#include "smallsh.h"

// TODO: cd implementation
//      if argument !=2, error print
// TODO: exit
//      shell exiting function
// TODO: '>' Handling
//      writes standard output in particular file.
//      should work with '>', ';', '&' used in same time
// TODO: '&' problem finding
//      (zombie, orphan)

static char inpbuf[MAXBUF];
static char tokbuf[2 * MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;

static char special[] = {' ', '\t', '&', ';', '\n', '\0'};

// p: keyboard input (in main)
// sets user input in inpbuf
// count: number of letter in inpbuf (p)
int userin(char *p) {
    int c, count;
    ptr = inpbuf;
    tok = tokbuf;

    printf("%s", p);
    count = 0;

    // Getting letter by letter
    while (1) {
        if ((c = getchar()) == EOF) return EOF;

        if (count < MAXBUF) inpbuf[count++] = c;
        if (c == '\n' && count < MAXBUF) {
            inpbuf[count] = '\0';
            return count;
        }

        if (c == '\n' || count >= MAXBUF) {
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s", p);
        }
    }
}

// Slices result from userin(inpbuf) by token parts,
// sets result in tokbuf
int gettok(char **outptr) {
    int type;
    *outptr = tok;
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    *tok++ = *ptr;
    switch (*ptr++) {
        case '\n':
            type = EOL;
            break;
        case '&':
            type = AMPERSAND;
            break;
        case ';':
            type = SEMICOLON;
            break;
        default:
            type = ARG;
            while (inarg(*ptr)) *tok++ = *ptr++;
    }
    *tok++ = '\0';
    return type;
}

// returns 0: if given input c is special letter
// returns 1: if not special letter
int inarg(char c) {
    char *wrk;

    for (wrk = special; *wrk; wrk++) {
        if (c == *wrk) return 0;
    }
    return 1;
}

// Decides option from tokens from gettok.
// Runs command by calling runcommand
void procline() {
    char *arg[MAXARG + 1];
    int toktype, type;
    int narg = 0;
    for (;;) {
        switch (toktype = gettok(&arg[narg])) {
            case ARG:
                if (narg < MAXARG) narg++;
                break;
            case EOL:
            case SEMICOLON:
            case AMPERSAND:
                if (toktype == AMPERSAND)
                    type = BACKGROUND;
                else
                    type = FOREGROUND;
                // MARK: Real command running is here
                if (narg != 0) {
                    arg[narg] = NULL;
//                    for (int i = 0; i < narg; i++) {
//                        printf("narg[%d]: %s\n", i, arg[i]);
//                    }
                    char *first = arg[0];
                    if (!strcmp(first, "cd\0")) {
                        chDir(arg);
                    } else if (!strcmp(first, "exit()\0") || !strcmp(first, "return\0")) {
                        printf("BYE!\n");
                        exit(1);
                    } else {
                        arg[narg] = NULL;
                        runcommand(arg, type);
                    }
                }
                if (toktype == EOL) return;
                narg = 0;
                break;
        }
    }
}

// runs command
int runcommand(char **cline, int where) {
    pid_t pid;
    int status;

    switch (pid = fork()) {
        case -1:
            perror("smallsh");
            return -1;
        case 0:
            execvp(*cline, cline);
            perror(*cline);
            exit(1);
    }

    if (where == BACKGROUND) {
        printf("[Process id] %d\n", pid);
        return 0;
    }
    if (waitpid(pid, &status, 0) == -1)
        return -1;
    else
        return status;
}

void chDir(char **arg) {
    char directoryBuffer[MAXBUF];
    char pathname[MAXBUF];

    getcwd(directoryBuffer, MAXBUF);

    strcpy(pathname, directoryBuffer);
    strcat(pathname, "/");
    strcat(pathname, arg[1]);

    chdir(pathname);
}