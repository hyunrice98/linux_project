#include "smallsh.h"

// TODO: '&' 처리. 고아 처리 완료후 cwd 출력.
// 입력 prompt 에 "Command>" 대신 "현재 디렉토리" 표시
// TODO: SIGINT 처리. Foreground process들 종료. smallsh는 종료되지 않고 prompt 다시 표시
// TODO: '|' (pipe) 처리. command1 | command2
//       -> command1의 STDOUT 이 command2의 STDIN 이 되도록 두 프로세스를 연결

static char inpbuf[MAXBUF];
static char tokbuf[2 * MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;

static char special[] = {' ', '\t', '&', ';', '\n', '\0'};

char *argv1[MAXBUF] = {};
char *argv2[MAXBUF] = {};

// p: keyboard input (in main)
// sets user input in inpbuf
// count: number of letter in inpbuf (p)
int userin() {
    int c, count;
    ptr = inpbuf;
    tok = tokbuf;

    char currentDirectory[MAXBUF];
    getcwd(currentDirectory, MAXBUF);
    strcat(currentDirectory, "$ ");
    printf("%s", currentDirectory);
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
            printf("%s", currentDirectory);
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
// returns 1: if not speci-al letter
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
                if (narg != 0) {
                    arg[narg] = NULL;

                    char *first = arg[0];
                    // making cd exception
                    if (!strcmp(first, "cd\0")) {
                        chDir(arg, narg);
                    }
                        // exit || return exception
                    else if (!strcmp(first, "exit\0") ||
                             !strcmp(first, "exit()\0") ||
                             !strcmp(first, "return\0")) {
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

void zombieHandler(int a) {
    int status;
    wait(&status);
}

void catchSigint(int signo) {
    int status;
    kill(&status, signo);
}

void ignSigint(int signo) {
    signal(signo, SIG_IGN);
}

// runs command
int runcommand(char **cline, int where) {
    pid_t pid;
    int status;

    struct sigaction act;
    sigfillset(&act.sa_mask);
    act.sa_handler = zombieHandler;
    act.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &act, NULL);

    struct sigaction act2;
    act2.sa_handler = catchSigint;
    sigfillset(&act2.sa_mask);
    sigaction(SIGINT, &act2, NULL);

    for (int i = 0; i < MAXBUF; i++) {
        argv1[i] = NULL;
        argv2[i] = NULL;
    }
    int i, j;
    int hasPipe = 0;
    for (i = 0; cline[i] != NULL; i++) {
        argv1[i] = cline[i];
        if (!strcmp(cline[i], "|")) {
            argv1[i] = NULL;
            hasPipe = 1;
            break;
        }
    }
    if (hasPipe == 1) {
        for (j = i + 1; cline[j] != NULL; j++) {
            argv2[j - i - 1] = cline[j];
        }
        pipeHandler(argv1, argv2);
    } else {
        switch (pid = fork()) {
            case -1:
                perror("smallsh");
                return -1;
            case 0:
                fileOutput(cline);
                execvp(*cline, cline);
                perror(*cline);
                exit(1);
            default:
                // Below is 4 parent
                signal(SIGINT, ignSigint);
                if (where == BACKGROUND) {
                    printf("[Process id] %d\n", pid);
                    return 0;
                }

                if (waitpid(pid, &status, 0) == -1)
                    return -1;
                else
                    return status;
        }
    }
}

void chDir(char **arg, int narg) {
    if (narg != 2) {
        perror("cd command should have two arguments\n");
        return;
    }
    char newDirectory[MAXBUF];

    char currentDirectory[MAXBUF];
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

int pipeHandler() {
    int p[2];

    if (pipe(p) == -1) {
        perror("pipe call");
        return 1;
    }

    if (fork() == 0) {
        dup2(p[0], STDIN_FILENO);
        if (fork() == 0) {
            dup2(p[1], STDOUT_FILENO);
            execvp(argv1[0], argv1);
        }
        wait(NULL);
        execvp(argv2[0], argv2);
    }
}