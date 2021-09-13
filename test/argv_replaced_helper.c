#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int len0 = strlen(argv[0]);
    int len1 = argc < 2 ? 0 : strlen(argv[1]);
    if (argv[1] != NULL && strcmp(argv[0]+len0-len1, argv[1]) == 0)
        return 0;

    return 42;
}
