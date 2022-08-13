//
// Created by jinfan36 on 2022/8/13.
//

#include <stdio.h>
#include <stdlib.h>


struct ShellPathNode
{
    const char* shellPath;
    struct ShellPathNode* next = NULL;
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
    struct ShellPathNode *curNode = pathHead;
    while(curNode)
    {
        struct ShellPathNode *nextNode = curNode->next;
        free(curNode);
        curNode = nextNode;
    }
}



void printErrorMassageToStderr()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void freeDynamicMemory()
{
    freeShellPathList();
}


void processInstruction(char* instruction)
{

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
        printErrorMassageToStderr();
        exit(1);
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

    freeDynamicMemory();

    return 0;
}
