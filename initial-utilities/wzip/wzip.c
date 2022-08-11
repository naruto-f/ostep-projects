//
// Created by 123456 on 2022/8/11.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    char cur_char = '0';
    char prev_char = '1';
    char flag = 0;    //标志是不是读第一个字符
    int count = 1;
    for(int i = 1; i < argc; ++i)
    {
        FILE* fp = fopen(argv[i], "r");
        if(NULL == fp)
        {
            printf("wzip: cannot open file\n");
            exit(1);
        }

        while(fread((void*)&cur_char, 1, 1, fp))
        {
            if(cur_char == prev_char)
            {
                ++count;
            }
            else
            {
                if(!flag)
                {
                    flag = 1;
                }
                else
                {
                    fwrite((void*)&count, 4, 1, stdout);
                    fwrite((void*)&prev_char, 1, 1, stdout);
                }

                prev_char = cur_char;
                count = 1;
            }
        }

        fclose(fp);
    }
    fwrite((void*)&count, 4, 1, stdout);
    fwrite((void*)&prev_char, 1, 1, stdout);

    return 0;
}

