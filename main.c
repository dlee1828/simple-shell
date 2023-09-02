#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int PERMISSIONS = S_IRUSR | S_IWUSR | S_IXUSR;

void execute_command(char** argv) {
    char* output_file_name = NULL;
    char* input_file_name = NULL;

    int i = 0;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], ">") == 0) {
            // Output redirection
            output_file_name = argv[i + 1];
            argv[i] = NULL;
        } else if (strcmp(argv[i], "<") == 0) {
            // Input redirection
            input_file_name = argv[i + 1];
            argv[i] = NULL;
        }
        i++;
    }

    if (output_file_name != NULL) {
        int output_file_descriptor =
            open(output_file_name, O_CREAT | O_WRONLY, PERMISSIONS);
        dup2(output_file_descriptor, STDOUT_FILENO);
        close(output_file_descriptor);
    }
    if (input_file_name != NULL) {
        int input_file_descriptor = open(input_file_name, O_RDONLY);
        int d = dup2(input_file_descriptor, STDIN_FILENO);
        close(input_file_descriptor);
    }
    execvp(argv[0], argv);
}

void handle_single_command(char** argv) {
    pid_t pid = fork();

    if (pid < 0) {
        // error
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execute_command(argv);
    } else {
        // This block will be executed by the parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for the child to terminate
    }
}

void handle_piped_commands(char** left_argv, char** right_argv) {
    int pipe_fd[2];
}

void handle_command_line(char** tokens) {
    // Find a pipe
    char* left_argv[100];
    char* right_argv[100];
    bool has_pipe = false;

    int left_i = 0;
    int right_i = 0;
    int i = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "|") == 0) {
            has_pipe = true;
            continue;
        }

        if (!has_pipe) {
            left_argv[left_i] = tokens[i];
            left_i++;
        } else {
            right_argv[i] = tokens[i];
            right_i++;
        }
        i++;
    }

    left_argv[left_i] = NULL;
    right_argv[right_i] = NULL;

    if (has_pipe)
        handle_piped_commands(left_argv, right_argv);
    else
        handle_single_command(left_argv);
}

int main() {
    char* tokens[100];
    char* line = NULL;
    size_t size = 0;
    while (true) {
        printf("# ");
        getline(&line, &size, stdin);
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        char* token = strtok(line, " ");
        int tokens_index = 0;
        while (token != NULL) {
            tokens[tokens_index] = token;
            token = strtok(NULL, " ");
            tokens_index++;
        }
        tokens[tokens_index] = NULL;

        handle_command_line(tokens);
    }
}