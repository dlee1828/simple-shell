#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
    char* s = NULL;
    int x = strcmp(s, "hii");
    printf("%d\n", x);
    return 0;
}