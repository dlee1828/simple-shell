#include <stdio.h>
#include <unistd.h>

int main() {
    // char* argv[] = {"ls", "-l", NULL};
    char* argv[] = {"grep", "\"a\"", "file.txt", NULL};
    if (execvp(argv[0], argv) == -1) {
        perror("execvp");
        return 1;
    }
    return 0;
}