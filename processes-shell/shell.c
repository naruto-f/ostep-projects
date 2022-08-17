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
#include <dirent.h>
#include <errno.h>


#define MAX_ARG_NUM 5                //单条命令最大的参数数量
#define MAX_REDIRECTION_NUM  5       //单条命令允许的最大重定向文件数
#define MAX_COMMAND_PER_Line 20      //每行允许最多并行的命令数


/***                        内存管理函数                                 ***/
char* copyStringUsingMalloc(char* src)
{
    if(src == NULL)
    {
        return NULL;
    }

    char* dest = NULL;
    int destLen = strlen(src) + 1;
    if((dest = (char *) malloc(destLen)) == NULL)
    {
        return NULL;
    }
    memcpy(dest, src, destLen - 1);
    dest[destLen - 1] = '\0';

    return dest;
}

/***          管理shellpath的链表的数据结构及相关操作                     ***/
struct ShellPathNode {
    char *shellPath;
    struct ShellPathNode *next;
};

/* 管理shellpath的链表的头节点 */
struct ShellPathNode *pathHead;

void insertNewShellPath(char *path) {
    struct ShellPathNode *pathNode = NULL;
    pathNode = (struct ShellPathNode *) malloc(sizeof(struct ShellPathNode));
    if(pathNode == NULL)
    {
        printf("malloc filed\n");
        return;
    }
    pathNode->shellPath = path;
    pathNode->next = pathHead->next;
    pathHead->next = pathNode;
}

void initShellPath() {
    pathHead = (struct ShellPathNode *) malloc(sizeof(struct ShellPathNode));
    bzero(pathHead, sizeof(struct ShellPathNode));
    pathHead->next = NULL;

    char* necessaryPath = NULL;
    char buffer[5] = "/bin";
    necessaryPath = copyStringUsingMalloc(buffer);
    insertNewShellPath(necessaryPath);
}

void freeShellPathList() {
    struct ShellPathNode *curNode = pathHead->next;
    while (curNode) {
        struct ShellPathNode *nextNode = curNode->next;
        free(curNode->shellPath);
        curNode->shellPath = NULL;
        free(curNode);
        curNode = nextNode;
    }
    pathHead->next = NULL;
}

int isShellPathEmpty() {
    return (pathHead->next == NULL) ? 1 : 0;
}


/***       存放命令行解析得出的单条指令的命令和参数的数据结构                                         ***/
struct CommandInfo {
    char *command;
    int isBuiltInCmd;
    int isValidatyCmd;
    int argc;
    char* argv[MAX_ARG_NUM];
    int needRedirection;
    char* fileNameOfRedirection;
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
    for(int i = 0; i < curPosOfcmdInfo; ++i)
    {
        if(cmdInfoSet[i].isValidatyCmd && (cmdInfoSet[i].isBuiltInCmd != 1))
        {
            free(cmdInfoSet[i].command);
        }
    }

    bzero(&cmdInfoSet, MAX_COMMAND_PER_Line * sizeof(struct CommandInfo));
    curPosOfcmdInfo = 0;
}


void printErrorMassageToStderr() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void freeDynamicMemory() {
    freeShellPathList();
    clearCmdInfoSet();
    free(pathHead);
}

void exitForErrorWithoutMemoryLeak() {
    freeDynamicMemory();
    printErrorMassageToStderr();
    exit(1);
}

int checkvalidityOfNonBuiltInCommand(struct CommandInfo *cmdInfo) {
    if(cmdInfo->needRedirection == -1)
    {
        return -1;
    }

    struct ShellPathNode *workNode = pathHead->next;
    while (workNode) {
        struct ShellPathNode *nextNode = workNode->next;

        char *absolutePath = (char *) malloc(strlen(workNode->shellPath) + strlen(cmdInfo->command) + 1 + 1);
        sprintf(absolutePath, "%s%s%s%c", workNode->shellPath, "/", cmdInfo->command, '\0');
        if (access(absolutePath, X_OK) == 0) {
            cmdInfo->isValidatyCmd = 1;
            cmdInfo->command = absolutePath;
            return 0;
        }
        free(absolutePath);

        workNode = nextNode;
    }

    return -1;
}

int checkIsCommandBuiltIn(struct CommandInfo *cmdInfo) {
    if((strcmp(cmdInfo->command, "exit") && strcmp(cmdInfo->command, "cd") && strcmp(cmdInfo->command, "path")) == 0)
    {
        cmdInfo->isBuiltInCmd = 1;
        return 0;
    }

    return -1;
}

void processExit() {
    freeDynamicMemory();
    exit(0);
}

void processPath(int argc, char *argv[]) {
    freeShellPathList();

    if(argc == 1)
    {
        /* case: path 后面没有任何参数  */
        return;
    }

    for (int i = 1; i < argc; ++i) {
        char* newShellPath = NULL;

        if(argv[i][0] == '/')
        {
            /* 如果是绝对路径直接加进去 */
            newShellPath = copyStringUsingMalloc(argv[i]);
        }
        else
        {
            char *currentWorkDir = NULL;
            if((currentWorkDir = getcwd(NULL, 0)) == NULL)
            {
                printErrorMassageToStderr();
                continue;
            }

            char *absolutePath = (char *) malloc(strlen(currentWorkDir) + strlen(argv[i]) + 1);
            sprintf(absolutePath, "%s%s%s", currentWorkDir, "/", argv[i]);
            free(currentWorkDir);

            newShellPath = absolutePath;
        }
        insertNewShellPath(newShellPath);
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
        if(cmdInfo->argc > 1)
        {
            printErrorMassageToStderr();
            return;
        }

        processExit();
    } else if (strcmp(cmdInfo->command, "cd") == 0) {
        if (cmdInfo->argc != 2 || processCd(cmdInfo->argv[1]) == -1) {
            printErrorMassageToStderr();
        }
    } else {
        processPath(cmdInfo->argc, cmdInfo->argv);
    }
}

int needRedirection(char *instruction, char **redirectionFilesStart)
{
    int count = 0;
    *redirectionFilesStart = instruction;
    while(**redirectionFilesStart != '\0')
    {
        if(**redirectionFilesStart == '>')
        {
            if(count == 0)
            {
                return -1;
            }

            **redirectionFilesStart = '\0';
            ++*redirectionFilesStart;
            return 1;
        }

        ++*redirectionFilesStart;
        ++count;
    }

    return 0;
}

int copyFileContentToOtherFile(int srcFd, const char* destFileName)
{
    char buf[1024];
    bzero(buf, 1024);

    int destFd = open(destFileName, O_RDWR | O_CREAT | O_TRUNC);
    if(destFd < 0)
    {
        return -1;
    }

    int readCount = 0;
    while((readCount = read(srcFd, buf, 1024)) > 0)
    {
        if(write(destFd, buf, readCount) < 0)
        {
            return -1;
        }
        bzero(buf, 1024);
    }

    close(destFd);
    return 0;
}

void parseAnInstruction(char *instruction) {
    struct CommandInfo cmdInfo;
    bzero(&cmdInfo, sizeof(struct CommandInfo));
    char *tempPtr = NULL;
    int count = 0;

    while(*instruction == ' ' || *instruction == '\t')
    {
        ++instruction;
    }

    /* 判断是否需要重定向输出 */
    char* redirectionFilesStart = NULL;
    int need = needRedirection(instruction, &redirectionFilesStart);
    if(need == 1)
    {
        while(*redirectionFilesStart == ' ' || *redirectionFilesStart == '\t')
        {
            ++redirectionFilesStart;
        }

        int count = 0;
        while((tempPtr = strsep(&redirectionFilesStart, " \t")) != NULL)
        {
            if(strcmp(tempPtr, "")  == 0)
            {
                break;
            }

            if(count == 0)
            {
                cmdInfo.fileNameOfRedirection = tempPtr;
            }
            ++count;
        }

        if(count == 1) {
            cmdInfo.needRedirection = 1;
        }
        else {
            cmdInfo.needRedirection = -1;
        }

        tempPtr = NULL;
    }
    else if(need == -1)
    {
        cmdInfo.needRedirection = -1;
        cmdInfo.command = "error";
        insertNewCommandInfo(cmdInfo);
        return;
    }

    while ((tempPtr = strsep(&instruction, " \t")) != NULL) {
        if(strcmp(tempPtr, "") == 0)
        {
            break;
        }

        if (count == 0) {
            cmdInfo.command = tempPtr;
            cmdInfo.argv[cmdInfo.argc++] = tempPtr;
        } else {
            cmdInfo.argv[cmdInfo.argc++] = tempPtr;
        }

        ++count;
        tempPtr = NULL;
    }

    if (cmdInfo.command != NULL && strcmp(cmdInfo.command, "") != 0) {
        insertNewCommandInfo(cmdInfo);
    }
}


void parseALineToInstructions(char *instructionLine) {
    char *tempPtr = NULL;
    while ((tempPtr = strsep(&instructionLine, "&\n")) != NULL) {
        if(strcmp(tempPtr, "") == 0)
        {
            break;
        }

        parseAnInstruction(tempPtr);
        tempPtr = NULL;
    }
}


void processInstruction(char *instructionLine) {
    parseALineToInstructions(instructionLine);

    int childCount = 0;
    for (int i = 0; i != curPosOfcmdInfo; ++i) {
        struct CommandInfo *curCommand = &cmdInfoSet[i];

        if (checkIsCommandBuiltIn(curCommand) == 0) {
            runBuiltInCommand(curCommand);
            continue;
        } else if (isShellPathEmpty() || checkvalidityOfNonBuiltInCommand(curCommand) == -1) {
            /* 如果非内建命令不合法则报错后继续等待接收下一条指令 */
            printErrorMassageToStderr();
            continue;
        }

        pid_t pid = -1;
        if ((pid = fork()) < 0) {
            printErrorMassageToStderr();
            continue;
        }
        else if (pid == 0) {
            int redirectionFd = -1;
            if (curCommand->needRedirection == 1) {
                /* 如果需要重定向，则需要子进程在execv运行进程前将自己的stdout对应的fd(即1)关闭 */
                close(STDOUT_FILENO);

                redirectionFd = open(curCommand->fileNameOfRedirection, O_RDWR | O_CREAT | O_TRUNC);
                if (redirectionFd == -1) {
                    printErrorMassageToStderr();
                    exit(1);
                }
            }

            if (execv(curCommand->command, curCommand->argv) == -1) {
                exitForErrorWithoutMemoryLeak();
            }

            close(redirectionFd);
            exit(0);
        }
        else
        {
            ++childCount;
        }
    }

    while(childCount--)
    {
        wait(NULL);
    }

    clearCmdInfoSet();
}

void shellRun(FILE *fp, const char *prompt) {
    char* instructionLines[10] = { NULL };
    int instructionNum = 0;
    char *instructionLine = NULL;
    size_t zero = 0;

    while (getline(&instructionLine, &zero, fp) != -1) {
        instructionLines[instructionNum++] = copyStringUsingMalloc(instructionLine);
    }

    for(int i = 0; i < instructionNum; ++i)
    {
        instructionLine = instructionLines[i];
        processInstruction(instructionLine);
        free(instructionLine);
        instructionLine = NULL;
    }

    processExit();
}


void startCommandLineMode() {
    printf("wish> ");

    shellRun(stdin, "\nwish> ");
}


void startBatchMode(const char *batchFileName) {
    FILE *fp = fopen(batchFileName, "r");
    if (fp == NULL) {
        exitForErrorWithoutMemoryLeak();
    }

    shellRun(fp, "");
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        printErrorMassageToStderr();
        exit(1);
    }

    initShellPath();
    clearCmdInfoSet();

    if (argc == 1) {
        startCommandLineMode();
    } else {
        startBatchMode(argv[1]);
    }

    return 0;
}
