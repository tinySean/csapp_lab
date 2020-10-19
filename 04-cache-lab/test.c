#include <stdio.h>
#include <math.h>
int main()
{
    unsigned int address;
    while (1)
    {
        scanf("%x", &address);
        printf("%x:%d\n", address, address);
        printf("%x\n", address & ((int)pow(2, 4) - 1));
        printf("%x\n", address >> 4);
    }
}