//
// Created by 123456 on 2022/8/11.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 1024

int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        printf("wunzip: file1 [file2 ...]\n");
        exit(1);
    }

    for(int i = 1; i < argc; ++i)
    {
        FILE *fp = fopen(argv[i], "r");
        if(NULL == fp)
        {
            printf("wunzip: cannot open file\n");
            exit(1);
        }

        int count = 0;
        char c;

        while(fread(&count, 4, 1, fp) && fread(&c, 1, 1, fp))
        {
            for(int i = 0; i < count; ++i)
            {
                printf("%c", c);
            }
        }

        fclose(fp);
    }

    return 0;
}

