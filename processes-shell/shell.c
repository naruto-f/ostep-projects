//
// Created by jinfan36 on 2022/8/13.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#define MAX_ARG_NUM 5
#define MAX_PID_NUM 20

struct ShellPathNode
{
    const char* shellPath;
    struct ShellPathNode* next;
};

/* 管理shellpath的链表的头节点 */
struct ShellPathNode *pathHead;

void insertNewShellPath(const char* path)
{
    struct ShellPathNode* pathNode = (struct ShellPathNode*)malloc(sizeof(struct ShellPathNode));
    pathNode->shellPath = path;
    pathNode->next = pathHead->next;
    pathHead->next = pathNode;
}

void initShellPath()
{
    pathHead = (struct ShellPathNode*)malloc(sizeof(struct ShellPathNode));

    const char* necessaryPath = "/bin";
    insertNewShellPath(necessaryPath);
}

void freeShellPathList()
{
    struct ShellPathNode *curNode = pathHead->next;
    while(curNode)
    {
        struct ShellPathNode *nextNode = curNode->next;
        free(curNode);
        curNode = nextNode;
    }
    pathHead->next = NULL;
}

int isShellPathEmpty()
{
    return !(pathHead->next == NULL);
}



void printErrorMassageToStderr()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void freeDynamicMemory()
{
    freeShellPathList();
    free(pathHead);
}

void exitForErrorWithoutMemoryLeak()
{
    freeDynamicMemory();
    printErrorMassageToStderr();
    exit(1);
}

int checkvalidityOfNonBuiltInCommand(const char* command)
{
    struct ShellPathNode* workNode = pathHead->next;
    while(workNode)
    {
        struct ShellPathNode* nextNode = workNode->next;

        char* absolutePath = (char*)malloc(sizeof(strlen(workNode->shellPath) + strlen(command) + 1));
        sprintf(absolutePath, "%s%s%s", workNode->shellPath, "/", command);
        if(access(absolutePath, X_OK) == 0)
        {
            return 0;
        }
        free(absolutePath);

        workNode = nextNode;
    }

    return -1;
}

int checkIsCommandBuiltIn(const char* command)
{
    return !strcmp(command, "exit") && !strcmp(command, "exit") && !strcmp(command, "exit");
}

void processExit()
{
    freeDynamicMemory();
    exit(0);
}

void processPath(int argc, char* argv[])
{
    freeShellPathList();

    for(int i = 0; i < argc; ++i)
    {
        insertNewShellPath(argv[i]);
    }
}

int processCd(const char* dirName)
{
    if(chdir(dirName) == -1)
    {
        return -1;
    }

    return 0;
}

void runBuiltInCommand(const char* command, int argc, char* argv[])
{
    if(!strcmp(command, "exit"))
    {
        processExit();
    }
    else if(!strcmp(command, "cd"))
    {
        if(argc != 1 || processCd(argv[0]) == -1)
        {
            printErrorMassageToStderr();
        }
    }
    else
    {
        processPath(argc, argv);
    }
}

void processInstruction(char* instruction)
{
    char* argv[MAX_ARG_NUM] = { NULL };
    char* command = NULL;
    char* temp = NULL;
    int count = 0;
    int pidNum = 0;

    while((temp = strsep(&instruction, " ")) != NULL)
    {
        if(count == 0)
        {
            command = temp;
        }
        else
        {
            argv[count - 1] = temp;
        }

        ++count;
    }

    if(checkIsCommandBuiltIn(command) == 0)
    {
        runBuiltInCommand(command, 0, argv);
    }
    else if(checkvalidityOfNonBuiltInCommand(command) == -1 || !isShellPathEmpty())
    {
        /* 如果非内建命令不合法则直接退出shell */
        exitForErrorWithoutMemoryLeak();
    }

    pid_t pidSet[MAX_PID_NUM] = { 0 };
    unsigned int pidSetPos = 0;
    for(int i = 0; i < pidNum; ++i)
    {
        pid_t pid = fork();
        if(pid == -1)
        {
            exitForErrorWithoutMemoryLeak();
        }

        if(pid == 0)
        {
            if(execv(command, argv) == -1)
            {
                exitForErrorWithoutMemoryLeak();
            }
        }
        else
        {
            pidSet[pidSetPos++] = pid;
        }
    }

    int childErrorFlag = 0;
    for(int i = 0; i < pidSetPos; ++i)
    {
        if(waitpid(pidSet[i], NULL, WUNTRACED) != pidSet[i])
        {
            childErrorFlag = 1;
        }
    }

    if(childErrorFlag)
    {
        exitForErrorWithoutMemoryLeak();
    }
}

void shellRun(FILE *fp, const char* prompt)
{
    char* instruction = NULL;
    const size_t zero = 0;
    while(getline(&instruction, &zero, fp) != -1)
    {
        processInstruction(instruction);
        free(instruction);
        instruction = NULL;
        printf("%s", prompt);
    }
}


void startCommandLineMode()
{
    printf("prompt> ");

    shellRun(stdin, "\nprompt> ");
}


void startBatchMode(const char* batchFileName)
{
    FILE *fp = fopen(batchFileName, "r");
    if(fp == NULL)
    {
        exitForErrorWithoutMemoryLeak();
    }

    shellRun(fp, "");
}


int main(int argc, char* argv[])
{
    if(argc > 2)
    {
        printErrorMassageToStderr();
        exit(1);
    }

    initShellPath();

    if(argc == 1)
    {
        startCommandLineMode();
    }
    else
    {
        startBatchMode(argv[1]);
    }

    return 0;
}
