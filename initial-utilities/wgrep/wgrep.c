//
// Created by jinfan36 on 2022/8/10.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define BUFF_SIZE 1024


int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }
    else if(argc == 2)
    {
        /* 当没有指定搜索文件时从标准输入检索 */
        char buffer[BUFF_SIZE];
        while(fgets(buffer, BUFF_SIZE, stdin))
        {
            if(strstr(buffer, argv[1]))
            {
                printf("%s", buffer);
            }
        }

        return 0;
    }

    if(strcmp("", argv[1]) == 0)
    {
        return 0;
    }

    for(int i = 2; i < argc; ++i)
    {
        FILE *fp = fopen(argv[i], "r");
        if(NULL == fp)
        {
            printf("wgrep: cannot open file\n");
            exit(1);
        }

        char* buffer = NULL;

        size_t max_len = 0;
        while(getline(&buffer, &max_len, fp) != -1)
        {
            if(strstr(buffer, argv[1]))
            {
                printf("%s", buffer);
            }

            free(buffer);
            buffer = NULL;
            max_len = 0;
        }
    }

    return 0;
}
