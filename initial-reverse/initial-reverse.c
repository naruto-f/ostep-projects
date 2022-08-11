//
// Created by 123456 on 2022/8/11.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct Line
{
    char* line;
    struct Line* next;
};


/* 使用链表头插法将所有行逆序 */
void reverseLinesToAnotherFile(FILE* srcFp, FILE* destFp)
{
    /* 为了便于头插和遍历使用头结点，其next指针指向链表首节点 */
    struct Line *head = (struct Line*)malloc(sizeof(struct Line));
    head->next = NULL;

    char *cur_ptr = NULL;
    size_t size = 0;
    while(getline(&cur_ptr, &size, srcFp) != -1)
    {
        struct Line *cur_line = (struct Line*)malloc(sizeof(struct Line));
        cur_line->line = cur_ptr;
        cur_ptr = NULL;
        cur_line->next = head->next;
        head->next = cur_line;
    }
    free(cur_ptr);

    /* 遍历链表将所有行逆序输出 */
    struct Line *cur_line = head->next;
    free(head);

    while(cur_line)
    {
        struct Line *next_line = cur_line->next;
        fprintf(destFp, "%s", cur_line->line);
        free(cur_line->line);
        free(cur_line);
        cur_line = next_line;
    }
}


int main(int argc, char* argv[])
{
    if(argc > 3)
    {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    if(argc == 1)
    {
        reverseLinesToAnotherFile(stdin, stdout);
    }
    else if(argc == 2)
    {
        FILE *fp = fopen(argv[1], "r");
        if(NULL == fp)
        {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }

        reverseLinesToAnotherFile(fp, stdout);

        fclose(fp);
    }
    else
    {
        if(strcmp(argv[1], argv[2]) == 0)
        {
            fprintf(stderr, "reverse: input and output file must differ\n");
            exit(1);
        }

        FILE *srcFp = fopen(argv[1], "r");
        if(NULL == srcFp)
        {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[1]);
            exit(1);
        }

        FILE *destFp = fopen(argv[2], "w");
        if(NULL == destFp)
        {
            fprintf(stderr, "reverse: cannot open file '%s'\n", argv[2]);
            exit(1);
        }

        struct stat stat_in;
        struct stat stat_out;
        bzero(&stat_in, sizeof(struct stat));
        bzero(&stat_out, sizeof(struct stat));

        stat(argv[1], &stat_in);
        stat(argv[2], &stat_out);

        if(stat_in.st_ino == stat_out.st_ino)
        {
            fclose(srcFp);
            fclose(destFp);

            fprintf(stderr, "reverse: input and output file must differ\n");
            exit(1);
        }

        reverseLinesToAnotherFile(srcFp, destFp);

        fclose(srcFp);
        fclose(destFp);
    }

    return 0;
}

