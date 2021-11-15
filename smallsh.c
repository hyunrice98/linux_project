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
                // TODO: Problem with '&'. "ls -l &>file1; ls &> file2" not working
                if (toktype == AMPERSAND)
                    type = BACKGROUND;
                else
                    type = FOREGROUND;
                if (narg != 0) {
                    arg[narg] = NULL;

                    char *first = arg[0];
                    // making cd exception
                    if (!strcmp(first, "cd\0")) {
                        chDir(arg, narg);
                    }
                        // exit || return exception
                    else if (!strcmp(first, "exit\0") || !strcmp(first, "exit()\0") || !strcmp(first, "return\0")) {
                        exit(1);
                    }
                        // normal command execution
                    else {
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
            fileOutput(cline);
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

void chDir(char **arg, int narg) {
    if (narg != 2) {
        perror("cd command should have two arguments\n");
        return;
    }
    char currentDirectory[MAXBUF];
    char newDirectory[MAXBUF];

    getcwd(currentDirectory, MAXBUF);

    strcpy(newDirectory, currentDirectory);
    strcat(newDirectory, "/");
    strcat(newDirectory, arg[1]);

    chdir(newDirectory);
}

// if ">" found, dup file & clear [>, filepath] value
int fileOutput(char **cline) {
    int i, fd;

    for (i = 0; cline[i] != NULL; i++) {
        if (!strcmp(cline[i], ">")) break;
    }

    if (cline[i]) {
        char *filename = cline[i + 1];
        fd = open(filename, O_WRONLY | O_CREAT, 0644);
        if (fd == -1) {
            perror(strcat(cline[i + 1], " cannot be opened (fileOutput Error)"));
            return -1;
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);

        cline[i] = NULL;
        cline[i + 1] = NULL;

        for (; cline[i] != NULL; i++) cline[i] = cline[i + 2];
        cline[i] = NULL;
    }
    return 0;
}