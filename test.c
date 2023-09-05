#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* create_command_string(char** argv) {
    int totalLength = 0;
    for (int i = 0; argv[i] != NULL; i++) {
        totalLength += strlen(argv[i]);
        // if (argv[i + 1] != NULL) totalLength++;  // for spaces
    }

    char* result = (char*)malloc(totalLength + 1);
    if (!result) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }
    result[0] = '\0';

    for (int i = 0; argv[i] != NULL; i++) {
        strcat(result, argv[i]);
        if (argv[i + 1] != NULL) {
            strcat(result, " ");
        }
    }

    return result;
}

void test_func() {
    char* argv[4] = {"one", "two", "three", NULL};
    char* command_string = create_command_string(argv);
    printf("%s\n", command_string);
    free(command_string);
}

int main() { printf("%d\n", strlen(NULL)); }