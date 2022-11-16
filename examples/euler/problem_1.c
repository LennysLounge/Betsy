#include <stdio.h>
int main(int argc, char *argv[])
{
    size_t stack_000 = 0;
    size_t sum = stack_000;
    stack_000 = 1;
    size_t i = stack_000;
    while (1)
    {
        stack_000 = 10000000;
        size_t stack_001 = i;
        stack_000 = stack_000 > stack_001;
        if(stack_000 == 0)
            break;
        {
            stack_000 = 0;
            stack_001 = i;
            size_t stack_002 = 3;
            stack_001 = stack_001 % stack_002;
            stack_000 = stack_000 == stack_001;
            stack_001 = 0;
            stack_002 = i;
            size_t stack_003 = 5;
            stack_002 = stack_002 % stack_003;
            stack_001 = stack_001 == stack_002;
            stack_000 = stack_000 + stack_001;
            if (stack_000 != 0)
            {
                stack_000 = sum;
                stack_001 = i;
                stack_000 = stack_000 + stack_001;
                sum = stack_000;
            }
            stack_000 = i;
            stack_001 = 1;
            stack_000 = stack_000 + stack_001;
            i = stack_000;
        }
    }
    stack_000 = sum;
    printf("%zd\n", stack_000);
    return 0;
}
