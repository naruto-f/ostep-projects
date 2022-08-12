//
// Created by jinfan36 on 2022/8/12.
//


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_KEY 1234568

struct List_node
{
    char* malloc_ptr;
    struct List_node* next;
};

const char *databaseName = "database.txt";

/* 标志在一次请求中是否有写入的需求，如果有的话在程序结束之前会将数据写回数据库文件 */
char writeFlag = 0;

/* 由于这里只是为了通过测试用例，假设作为键(int类型)的范围不超过1024，且为非负值，所以使用数组来模拟map，在一些工作负载下会浪费空间，但会节约查找的时间 */
char* map[MAX_KEY] = { NULL };

/* 需要释放的动态内存链表的头结点 */
struct List_node* head;

void insertNodeToList(struct List_node* head, struct List_node* node)
{
    node->next = head->next;
    head->next = node;
}

int loadDbToMemory()
{
    FILE *dbFp = fopen(databaseName, "r");
    if(NULL == dbFp)
    {
        return -1;
    }

    char *temPtr = NULL;
    size_t size = 0;
    while(getline(&temPtr, &size, dbFp) != -1)
    {
        struct List_node* cur_node = (struct List_node*)malloc(sizeof(struct List_node));
        cur_node->malloc_ptr = temPtr;
        insertNodeToList(head, cur_node);

        char* memPtr = strsep(&temPtr, ",");
        int key = atoi(memPtr);
        //const char* value = memPtr + strlen(memPtr) + 1;
        map[key] = strsep(&temPtr, ",");

        temPtr = NULL;
    }
    free(temPtr);

    fclose(dbFp);
    return 0;
}

int isTwoCommandSame(const char *com1, const char *com2)
{
    return !strcmp(com1, com2);
}

void processGet(int key)
{
    if(map[key] == NULL)
    {
        fprintf(stdout, "%d not found\n", key);
        return;
    }

    printf("%d,%s", key, map[key]);
}

void processPut(int key, char* value)
{
//    if(strcmp(map[key], value) == 0)
//    {
//        return;
//    }
//    else if(map[key] == NULL)
//    {
//        map[key] = value;
//        writeFlag = 1;
//    }

    map[key] = value;
    writeFlag = 1;
}

void processAll()
{
    for(int i = 0; i < MAX_KEY; ++i)
    {
        if(map[i] != NULL)
        {
            printf("%d,%s\n", i, map[i]);
        }
    }
}

void processClear()
{
    FILE *dbFp = fopen(databaseName, "w");
    if(dbFp == NULL)
    {
        fprintf(stderr, "Db clear filed!\n");
    }

    fclose(dbFp);

    for(int i = 0; i < MAX_KEY; ++i)
    {
        map[i] = NULL;
    }
}

void processDelete(int key)
{
    if(map[key] == NULL)
    {
        fprintf(stderr, "Key %d not found\n", key);
        return;
    }

    map[key] = NULL;
    writeFlag = 1;
}

void processInstruction(char* instruction)
{
    const char* command = strsep(&instruction, ",");

    if(!(isTwoCommandSame(command, "p") || isTwoCommandSame(command, "g") || isTwoCommandSame(command, "a") ||
            isTwoCommandSame(command, "c") || isTwoCommandSame(command, "d")))
    {
        fprintf(stderr, "bad command %s", command);
        return;
    }
    else if(isTwoCommandSame(command, "p"))
    {
        int key = atoi(strsep(&instruction, ","));
        char* value = strsep(&instruction, ",");
        processPut(key, value);
    }
    else if(isTwoCommandSame(command, "g"))
    {
        int key = atoi(strsep(&instruction, ","));
        processGet(key);
    }
    else if(isTwoCommandSame(command, "a"))
    {
        processAll();
    }
    else if(isTwoCommandSame(command, "d"))
    {
        int key = atoi(strsep(&instruction, ","));
        processDelete(key);
    }
    else
    {
        processClear();
    }
}

int writeDataToDb()
{
    FILE *dbFp = fopen(databaseName, "w");
    if(NULL == dbFp)
    {
        return -1;
    }

    for(int i = 0; i < MAX_KEY; ++i)
    {
        if(map[i] != NULL)
        {
            fprintf(dbFp, "%d,%s\n", i, map[i]);
        }
    }

    return 0;
}

void freeDynamicMomery()
{
    struct List_node* cur_node = head->next;
    struct List_node* next_node = NULL;
    free(head);

    while(cur_node)
    {
        next_node = cur_node->next;
        free(cur_node->malloc_ptr);
        free(cur_node);
        cur_node = next_node;
    }
}

int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        return 0;
    }

    head = (struct List_node*)malloc(sizeof(struct List_node));

    int loadState = loadDbToMemory();
    if(loadState == -1)
    {
        fprintf(stderr, "kv: cannot open db file\n");
        exit(1);
    }

    for(int i = 1; i < argc; ++i)
    {
        processInstruction(argv[i]);
    }

    if(writeFlag)
    {
        int writebackState = writeDataToDb();
        if(writebackState == -1)
        {
            fprintf(stderr, "Failed to write data to database\n");
            exit(1);
        }
    }

    freeDynamicMomery();

    return 0;
}
