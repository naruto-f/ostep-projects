//
// Created by jinfan36 on 2022/8/10.
//

#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 500



int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        /* 如果没有要读的文件，则直接返回，这与cat的实际行为不同，实际上不带参数的cat将从标准输入(如键盘)种读入直到遇到EOF(ctrl+d), 然后将读入的内容显示在标准输出 */
        return 0;
    }

    for(int i = 1; i < argc; ++i)
    {
        FILE *fp = fopen(argv[i], "r");
        if(NULL == fp)
        {
            printf("wcat: cannot open file\n");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        //bzero(buffer, BUFFER_SIZE);
        while(fgets(buffer, BUFFER_SIZE, fp))
        {
            printf("%s", buffer);
        }

        fclose(fp);
    }

    return 0;
}







