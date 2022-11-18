#include <stdio.h>
int main(int argc, char *argv[])
{
    size_t stack_000 = 10;
    size_t i = stack_000;
    while (1)
    {
        stack_000 = i;
        size_t stack_001 = 0;
        stack_000 = stack_000 > stack_001;
        if(stack_000 == 0)
            break;
        {
            stack_000 = i;
            printf("%zd\n", stack_000);
            stack_000 = i;
            stack_001 = 1;
            stack_000 = stack_000 - stack_001;
            i = stack_000;
        }
    }
    return 0;
}
