//
// Created by jinfan36 on 2022/8/13.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAX_ARG_NUM 5                //单条命令最大的参数数量
#define MAX_COMMAND_PER_Line 20      //每行允许最多并行的命令数


/***          管理shellpath的链表的数据结构及相关操作                     ***/
struct ShellPathNode {
    const char *shellPath;
    struct ShellPathNode *next;
};

/* 管理shellpath的链表的头节点 */
struct ShellPathNode *pathHead;

void insertNewShellPath(const char *path) {
    struct ShellPathNode *pathNode = (struct ShellPathNode *) malloc(sizeof(struct ShellPathNode));
    pathNode->shellPath = path;
    pathNode->next = pathHead->next;
    pathHead->next = pathNode;
}

void initShellPath() {
    pathHead = (struct ShellPathNode *) malloc(sizeof(struct ShellPathNode));

    const char *necessaryPath = "/bin";
    insertNewShellPath(necessaryPath);
}

void freeShellPathList() {
    struct ShellPathNode *curNode = pathHead->next;
    while (curNode) {
        struct ShellPathNode *nextNode = curNode->next;
        free(curNode);
        curNode = nextNode;
    }
    pathHead->next = NULL;
}

int isShellPathEmpty() {
    return !(pathHead->next == NULL);
}


/***       存放命令行解析得出的单条指令的命令和参数的数据结构                                         ***/
struct CommandInfo {
    char *command;
    int argc;
    char *argv[MAX_ARG_NUM];
    int needRedirection;
    char *fileNameOfRedirection;
};

struct CommandInfo cmdInfoSet[MAX_COMMAND_PER_Line];
unsigned int curPosOfcmdInfo = 0;

void insertNewCommandInfo(struct CommandInfo cmdInfo) {
    if (curPosOfcmdInfo == MAX_COMMAND_PER_Line) {
        return;
    }

    cmdInfoSet[curPosOfcmdInfo++] = cmdInfo;
}

void clearCmdInfoSet() {
    bzero(&cmdInfoSet, '\0');
    curPosOfcmdInfo = 0;
}


void printErrorMassageToStderr() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void freeDynamicMemory() {
    freeShellPathList();
    free(pathHead);
}

void exitForErrorWithoutMemoryLeak() {
    freeDynamicMemory();
    printErrorMassageToStderr();
    exit(1);
}

int checkvalidityOfNonBuiltInCommand(const struct CommandInfo *cmdInfo) {
    struct ShellPathNode *workNode = pathHead->next;
    while (workNode) {
        struct ShellPathNode *nextNode = workNode->next;

        char *absolutePath = (char *) malloc(sizeof(strlen(workNode->shellPath) + strlen(cmdInfo->command) + 1));
        sprintf(absolutePath, "%s%s%s", workNode->shellPath, "/", cmdInfo->command);
        if (access(absolutePath, X_OK) == 0) {
            return 0;
        }
        free(absolutePath);

        workNode = nextNode;
    }

    return -1;
}

int checkIsCommandBuiltIn(const struct CommandInfo *cmdInfo) {
    return strcmp(cmdInfo->command, "exit") && strcmp(cmdInfo->command, "cd") && strcmp(cmdInfo->command, "path");
}

void processExit() {
    freeDynamicMemory();
    exit(0);
}

void processPath(int argc, char *argv[]) {
    freeShellPathList();

    for (int i = 0; i < argc; ++i) {
        insertNewShellPath(argv[i]);
    }
}

int processCd(const char *dirName) {
    if (chdir(dirName) == -1) {
        return -1;
    }

    return 0;
}

void runBuiltInCommand(struct CommandInfo *cmdInfo) {
    if (strcmp(cmdInfo->command, "exit") == 0) {
        processExit();
    } else if (strcmp(cmdInfo->command, "cd") == 0) {
        if (cmdInfo->argc != 1 || processCd(cmdInfo->argv[0]) == -1) {
            printErrorMassageToStderr();
        }
    } else {
        processPath(cmdInfo->argc, cmdInfo->argv);
    }
}

void parseAnInstruction(char *instruction) {
    struct CommandInfo cmdInfo;
    char *tempPtr = NULL;
    int count = 0;

    while ((tempPtr = strsep(&instruction, " \t")) != NULL) {
        if (count == 0) {
            cmdInfo.command = tempPtr;
        } else if (strcmp(tempPtr, ">") == 0) {
            cmdInfo.needRedirection = 1;
        } else if (cmdInfo.needRedirection == 1) {
            cmdInfo.fileNameOfRedirection = tempPtr;
            break;
        } else {
            cmdInfo.argv[cmdInfo.argc++] = tempPtr;
        }

        ++count;
        tempPtr = NULL;
    }

    if (cmdInfo.command != NULL) {
        insertNewCommandInfo(cmdInfo);
    }
}


void parseALineToInstructions(char *instructionLine) {
    char *tempPtr = NULL;
    while ((tempPtr = strsep(&instructionLine, "&")) != NULL) {
        parseAnInstruction(tempPtr);
        tempPtr = NULL;
    }
}


void processInstruction(char *instructionLine) {
    parseALineToInstructions(instructionLine);

    for (int i = 0; i != curPosOfcmdInfo; ++i) {
        struct CommandInfo *curCommand = &cmdInfoSet[i];

        if (checkIsCommandBuiltIn(curCommand) == 0) {
            runBuiltInCommand(curCommand);
            continue;
        } else if (checkvalidityOfNonBuiltInCommand(curCommand) == -1 || !isShellPathEmpty()) {
            /* 如果非内建命令不合法则报错后继续等待接收下一条指令 */
            printErrorMassageToStderr();
            continue;
        }

        pid_t pid = fork();
        if (pid == -1) {
            printErrorMassageToStderr();
            continue;
        }

        if (pid == 0) {
            int redirectionFd = -1;
            if (curCommand->needRedirection == 1) {
                /* 如果需要重定向，则需要子进程在execv运行进程前将自己的stdout对应的fd(即1)关闭 */
                close(STDOUT_FILENO);

                redirectionFd = open(curCommand->fileNameOfRedirection, O_RDWR);
                if (redirectionFd == -1) {
                    printErrorMassageToStderr();
                    continue;
                }
            }

            if (execv(curCommand->command, curCommand->argv) == -1) {
                printErrorMassageToStderr();
                continue;
            }

            close(redirectionFd);
        } else {
            if (waitpid(pid, NULL, WUNTRACED) != pid) {
                printErrorMassageToStderr();
                continue;
            }
        }
    }

    clearCmdInfoSet();
}

void shellRun(FILE *fp, const char *prompt) {
    char *instructionLine = NULL;
    size_t zero = 0;
    while (getline(&instructionLine, &zero, fp) != -1) {
        char *needFree = instructionLine;
        processInstruction(instructionLine);
        free(needFree);
        instructionLine = NULL;
        printf("%s", prompt);
    }
    free(instructionLine);

    fclose(fp);
    processExit();
}


void startCommandLineMode() {
    printf("wish> ");

    shellRun(stdin, "\nwish> ");
}


void startBatchMode(const char *batchFileName) {
    if (access(batchFileName, F_OK) == -1) {
        printf("file not fount!\n");
        exit(1);
    }

    FILE *fp = fopen("tests/1.in", "r");
    if (fp == NULL) {
        exitForErrorWithoutMemoryLeak();
    }

    shellRun(fp, "");
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        printErrorMassageToStderr();
        exit(1);
    }

    initShellPath();

    if (argc == 1) {
        startCommandLineMode();
    } else {
        //startBatchMode(argv[1]);
        startBatchMode("tests/1.in");
    }

    return 0;
}
