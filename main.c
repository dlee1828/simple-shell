#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Define DEBUG mode
#define DEBUG 0

// Debug print macro
#if DEBUG
#define debug_print(...) printf(__VA_ARGS__)
#else
#define debug_print(...) /* nothing */
#endif

int PERMISSIONS = S_IRUSR | S_IWUSR | S_IXUSR;

struct CurrentForegroundProcess {
    pid_t pid;
    char** argv;
} current_fg_process;

void reset_current_fg_process() {
    current_fg_process.argv = NULL;
    current_fg_process.pid = -1;
}

void set_current_fg_process(pid_t pid, char** argv) {
    current_fg_process.pid = pid;
    current_fg_process.argv = argv;
}

struct Job {
    pid_t pid;
    bool is_running;
    char* command;
};

struct Job jobs[20];

void initialize_jobs_array() {
    for (int i = 0; i < 20; i++) {
        struct Job job = {
            .is_running = false,
            .pid = -1,  // pid of -1 signifies an empty array element
            .command = NULL,
        };
        jobs[i] = job;
    }
}

void add_job(struct Job job) {
    for (int i = 0; i < 20; i++) {
        if (jobs[i].pid < 0) {
            jobs[i] = job;
            return;
        }
    }
    printf("Jobs array is at maximum capacity\n");
}

char* create_command_string(char** argv) {
    int totalLength = 0;
    for (int i = 0; argv[i] != NULL; i++) {
        totalLength += strlen(argv[i]);
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

void print_jobs() {
    for (int i = 0; i < 20 && jobs[i].pid >= 0; i++) {
        int job_num = i + 1;
        char* recency = (i == 20 || jobs[i + 1].pid < 0) ? "+" : "-";
        char* status = jobs[i].is_running ? "Running" : "Stopped";
        char* command = jobs[i].command;

        printf("[%d] %s %s       %s\n", job_num, recency, status, command);
    }
}

void execute_command(char** argv) {
    debug_print("Executing command with arguments:\n");
    for (int j = 0; argv[j] != NULL; j++) {
        debug_print("%s\n", argv[j]);
    }
    debug_print("---\n");

    char* output_file_name = NULL;
    char* input_file_name = NULL;
    char* error_file_name = NULL;

    int i = 0;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], ">") == 0) {
            // Output redirection
            output_file_name = argv[i + 1];
            argv[i] = NULL;
        } else if (strcmp(argv[i], "2>") == 0) {
            // Error redirection
            error_file_name = argv[i + 1];
            argv[i] = NULL;
        } else if (strcmp(argv[i], "<") == 0) {
            // Input redirection
            input_file_name = argv[i + 1];
            argv[i] = NULL;
        }
        i++;
    }

    if (output_file_name != NULL) {
        debug_print("Setting output file\n");
        int output_file_descriptor =
            open(output_file_name, O_CREAT | O_WRONLY, PERMISSIONS);
        dup2(output_file_descriptor, STDOUT_FILENO);
        close(output_file_descriptor);
    }
    if (input_file_name != NULL) {
        debug_print("Setting input file\n");
        int input_file_descriptor = open(input_file_name, O_RDONLY);
        if (input_file_descriptor < 0) {
            printf("Input file does not exist.\n");
            return;
        }
        dup2(input_file_descriptor, STDIN_FILENO);
        close(input_file_descriptor);
    }
    if (error_file_name != NULL) {
        debug_print("Setting error file\n");
        int error_file_descriptor =
            open(error_file_name, O_CREAT | O_WRONLY, PERMISSIONS);
        dup2(error_file_descriptor, STDERR_FILENO);
        close(error_file_descriptor);
    }
    execvp(argv[0], argv);
}

void handle_piped_commands(char** left_argv, char** right_argv, char** argv,
                           bool is_background) {
    debug_print("Entered handle_piped_commands\n");
    debug_print("Left command: %s, right command: %s\n", left_argv[0],
                right_argv[0]);

    // Set stdout of left to pipe_fd[1]
    // Set stdin of right pipe to pipe_fd[0]

    pid_t pid_1 = fork();
    if (pid_1 < 0) {
        // error
        printf("Fork 1 failed\n");
        exit(EXIT_FAILURE);
    } else if (pid_1 == 0) {
        // Child
        // We then want to fork again to execute both processes
        signal(SIGINT, SIG_DFL);   // Reset handler to default
        signal(SIGTSTP, SIG_DFL);  // Reset handler to default

        int pipe_fd[2];
        if (pipe(pipe_fd) < 0) {
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }

        int pid_2 = fork();
        if (pid_2 < 0) {
            printf("Fork 2 failed\n");
            exit(EXIT_FAILURE);
        } else if (pid_2 == 0) {
            // Child - Left command
            debug_print("Entered process for left command\n");

            close(pipe_fd[0]);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);
            execute_command(left_argv);
            // debug_print("Reached end of left command\n");
        } else {
            // Parent - Right command
            debug_print("Entered process for right command\n");

            close(pipe_fd[1]);
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);
            execute_command(right_argv);
        }
    } else {
        // Parent
        set_current_fg_process(pid_1, argv);
        int status;
        if (is_background) {
            struct Job job = {.is_running = true,
                              .pid = pid_1,
                              .command = create_command_string(argv)};
            add_job(job);
        } else
            waitpid(pid_1, &status, WUNTRACED);
        reset_current_fg_process();
    }
}

void handle_single_command(char** argv, bool is_background) {
    debug_print("Entered function handle_single_command\n");
    pid_t pid = fork();

    if (pid < 0) {
        // error
        printf("Fork failed\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);   // Reset handler to default
        signal(SIGTSTP, SIG_DFL);  // Reset handler to default
        execute_command(argv);
    } else {
        // Set the child as the current foreground process
        set_current_fg_process(pid, argv);
        int status;

        // If it's a bg process, add it to the jobs list
        // Otherwise, wait
        if (is_background) {
            struct Job job = {.is_running = true,
                              .pid = pid,
                              .command = create_command_string(argv)};
            add_job(job);
        } else
            waitpid(pid, &status, WUNTRACED);
        reset_current_fg_process();
    }
}

void handle_command_line(char** tokens) {
    debug_print("Entered function handle_command_line\n");

    // Check if it's a background process
    bool is_background = false;
    int i = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "&") == 0 && tokens[i + 1] == NULL) {
            is_background = true;
            tokens[i] = NULL;
            break;
        }
        i++;
    }

    // Find a pipe
    char* left_argv[100];
    char* right_argv[100];
    bool has_pipe = false;

    int left_i = 0;
    int right_i = 0;
    i = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "|") == 0) {
            has_pipe = true;
        } else if (!has_pipe) {
            left_argv[left_i] = tokens[i];
            left_i++;
        } else {
            right_argv[right_i] = tokens[i];
            right_i++;
        }
        i++;
    }

    left_argv[left_i] = NULL;
    right_argv[right_i] = NULL;

    if (has_pipe)
        handle_piped_commands(left_argv, right_argv, tokens, is_background);
    else
        handle_single_command(left_argv, is_background);
}

void handle_sigint() {
    debug_print("   SIGINT\n");
    if (current_fg_process.pid >= 0) {
        kill(current_fg_process.pid, SIGINT);
        printf("\n");
    }
}

void handle_sigtstp() {
    debug_print("   SIGTSTP\n");
    if (current_fg_process.pid >= 0) {
        kill(current_fg_process.pid, SIGTSTP);

        // Add to jobs list
        struct Job job = {
            .command = create_command_string(current_fg_process.argv),
            .is_running = false,
            .pid = current_fg_process.pid,
        };
        add_job(job);

        printf("\n");
    }
}

int main() {
    char* tokens[100];
    char* line = NULL;
    size_t size = 0;

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    initialize_jobs_array();

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

        if (tokens[0] != NULL && strcmp(tokens[0], "q") == 0)
            return EXIT_SUCCESS;
        else if (tokens[0] != NULL && strcmp(tokens[0], "jobs") == 0 &&
                 tokens[1] == NULL) {
            print_jobs();
        } else
            handle_command_line(tokens);
    }
}