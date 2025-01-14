#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#define PIPE_READ 0
#define PIPE_WRITE 1
#define LINE_BUFSIZE 1024
#define TOKEN_BUFSIZE 64
#define TOKEN_DELIM " \t\n\r"
#define EXIT_ALLOCATION_FAILURE 1
#define EXIT_FORKING_FAILURE 2
#define EXIT_OPEN_FAILURE 3

#define ITALIC "\x1b[3m"
#define RED "\x1b[0;31m"
#define BLUE "\x1b[0;34m"
#define GREEN "\x1b[0;32m"
#define BOLD "\x1b[1m"
#define RESET "\x1b[0m"

void echo(char** args);
void shell(const char* username);
char* read_line(void);
char** split_line(char* line);
int my_system(char* const command);
void my_chmod(char** args);
void find(char** args);
void tee(char** args);
void watch(char** args);
int login(char* const user, char* const password);
char* login_handler(void);
int handle_pipe(char* command);

void shell(const char* username) {
    char *line;
    int status;

    do {
        printf(BOLD GREEN "%s" RESET "@" BLUE "myshell" RESET BOLD "> " RESET, username);
        line = read_line();
        status = my_system(line);

        free(line);

    }while(status);
}

char* read_line(void) {
    int bufsize = LINE_BUFSIZE;
    int position = 0;
    char* buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, RED "[sh]: allocation error\n" RESET);
        exit(EXIT_ALLOCATION_FAILURE);
    }

    while(1) {
        c = getchar();
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        }else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += LINE_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, RED "[sh]: allocation error\n" RESET);
                exit(EXIT_ALLOCATION_FAILURE);
            }
        }
    }
}

char** split_line(char* line) {
    int bufsize = TOKEN_BUFSIZE;
    int position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;

    if (!tokens) {
        fprintf(stderr, RED "[sh]: allocation error\n" RESET);
        exit(EXIT_ALLOCATION_FAILURE);
    }

    token = strtok(line, TOKEN_DELIM);
    while(token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOKEN_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, RED "[sh]: allocation error\n" RESET);
                exit(EXIT_ALLOCATION_FAILURE);
            }
        }
        token = strtok(NULL, TOKEN_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int my_system(char* const command) {

    if (handle_pipe(command)) {
        return 1;
    }

    char** args = split_line(command);
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (strcmp(args[0], "chmod") == 0) {
            my_chmod(args);
        } else if(strcmp(args[0], "find") == 0) {
            find(args);
        } else if(strcmp(args[0], "tee") == 0) {
            tee(args);
        } else if(strcmp(args[0], "watch") == 0) {
            watch(args);
        } else if(strcmp(args[0], "echo") == 0) {
            echo(args);
        }
        exit(0);
    } else if (pid < 0) {
        fprintf(stderr, RED "[sh]: fork error\n" RESET);
        exit(EXIT_FORKING_FAILURE);
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    free(args);
    return 1;
}

void read_lines_from_file(const char* filename, char users[][LINE_BUFSIZE], char passwords[][LINE_BUFSIZE], int* user_count) {
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, RED "[sh]: open file error\n" RESET);
        exit(EXIT_OPEN_FAILURE);
    }

    char buffer[LINE_BUFSIZE];
    *user_count = 0;

    while (fgets(buffer, LINE_BUFSIZE, file)) {
        buffer[strcspn(buffer, "\n")] = 0;

        char* username = strtok(buffer, " ");
        char* password = strtok(NULL, " ");

        if (username && password) {
            strncpy(users[*user_count], username, LINE_BUFSIZE - 1);
            strncpy(passwords[*user_count], password, LINE_BUFSIZE - 1);
            users[*user_count][LINE_BUFSIZE - 1] = '\0';
            passwords[*user_count][LINE_BUFSIZE - 1] = '\0';
            (*user_count)++;
        }
    }

    fclose(file);
}

int login(char* const user, char* const password) {
    char users[100][LINE_BUFSIZE];
    char passwords[100][LINE_BUFSIZE];
    int user_count = 0;

    read_lines_from_file("users.txt", users, passwords, &user_count);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i], user) == 0) {
            if (strcmp(passwords[i], password) == 0) {
                return 1;
            }
            return 0;
        }
    }

    return 0;
}

char* login_handler(void) {
    char username[LINE_BUFSIZE];
    char passwd[LINE_BUFSIZE];
    char* logged_user = malloc(sizeof(char) * LINE_BUFSIZE);

    if (!logged_user) {
        fprintf(stderr, RED "[sh]: allocation error\n" RESET);
        exit(EXIT_ALLOCATION_FAILURE);
    }

    while (1) {
        printf(ITALIC "Username: " RESET);
        if (fgets(username, LINE_BUFSIZE, stdin) != NULL) {
            username[strcspn(username, "\n")] = 0;
        }

        printf(ITALIC "Password: " RESET);
        if (fgets(passwd, LINE_BUFSIZE, stdin) != NULL) {
            passwd[strcspn(passwd, "\n")] = 0;
        }

        if (login(username, passwd)) {
            strcpy(logged_user, username);
            printf( GREEN "Login successful! Welcome %s!\n" RESET, username);
            return logged_user;
        } else {
            printf(RED "Login failed! Please try again.\n" RESET);
        }
    }
}

void my_chmod(char** args) {
    if (args[1] == NULL || args[2] == NULL) {
        fprintf(stderr, RED "[sh]: chmod: missing operand\n" RESET);
        return;
    }

    char* endptr;
    long mode = strtol(args[1], &endptr, 8);

    if (*endptr != '\0' || mode < 0 || mode > 0777) {
        fprintf(stderr, RED "[sh]: chmod: invalid mode: '%s'\n" RESET, args[1]);
        return;
    }

    for (int i = 2; args[i] != NULL; i++) {
        struct stat file_stat;
        
        if (stat(args[i], &file_stat) == -1) {
            fprintf(stderr, RED "[sh]: chmod: cannot access '%s': %s\n" RESET, args[i], strerror(errno));
            continue;
        }

        mode_t new_mode = (file_stat.st_mode & ~(S_IRWXU | S_IRWXG | S_IRWXO)) | ((mode_t)mode & 0777);

        int fd = open(args[i], O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, RED "[sh]: chmod: cannot open '%s': %s\n" RESET, args[i], strerror(errno));
            continue;
        }

        if (fchmod(fd, new_mode) == -1) {
            fprintf(stderr, RED "[sh]: chmod: cannot change permissions of '%s': %s\n" RESET, args[i], strerror(errno));
        }

        close(fd);
    }
}

void find(char** args) {
    char *path = ".";
    char *name = NULL;
    char *type = NULL;

    for (int i = 1; args[i] != NULL; i++) {
        if (strcmp(args[i], "-name") == 0) {
            if (args[i + 1] != NULL) {
                name = args[i + 1];
                i++;
            } else {
                fprintf(stderr, RED "[sh]: -name requires an argument\n" RESET);
                return;
            }
        } else if (strcmp(args[i], "-type") == 0) {
            if (args[i + 1] != NULL) {
                type = args[i + 1];
                i++;
            } else {
                fprintf(stderr, RED "[sh]: -type requires an argument\n" RESET);
                return;
            }
        } else {
            path = args[i];
        }
    }

    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror(RED "[sh]: cannot open directory" RESET);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        if (name && strstr(entry->d_name, name) == NULL) continue;

        if (type) {
            struct stat st;
            char fullpath[LINE_BUFSIZE];
            snprintf(fullpath, LINE_BUFSIZE, "%s/%s", path, entry->d_name);

            if (stat(fullpath, &st) == -1) continue;

            if ((type[0] == 'd' && !S_ISDIR(st.st_mode)) || (type[0] == 'f' && !S_ISREG(st.st_mode))) {
                continue;
            }
        }

        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

void tee(char** args) {
    int append_mode = 0;
    int print_help = 0;
    char buffer[LINE_BUFSIZE];
    FILE* files[TOKEN_BUFSIZE];
    int file_count = 0;

    for (int i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            if (args[i][1] == '-') {
                if (strcmp(args[i], "--help") == 0) {
                    print_help = 1;
                } else {
                    fprintf(stderr, RED "[sh]: Unknown option %s\n" RESET, args[i]);
                    return;
                }
            } else {
                for (int j = 1; args[i][j] != '\0'; j++) {
                    switch (args[i][j]) {
                        case 'a':
                            append_mode = 1;
                            break;
                        default:
                            fprintf(stderr, RED "[sh]: Unknown option -%c\n" RESET, args[i][j]);
                            return;
                    }
                }
            }
        } else {
            files[file_count] = fopen(args[i], append_mode ? "a" : "w");
            if (files[file_count] == NULL) {
                fprintf(stderr, RED "[sh]: Cannot open file %s\n" RESET, args[i]);
                continue;
            }
            file_count++;
        }
    }

    if (print_help) {
        printf("Usage: tee [OPTION]... [FILE]...\n");
        printf("Copy standard input to each FILE, and also to standard output.\n\n");
        printf("  -a, --append              append to the given FILEs, do not overwrite\n");
        printf("  --help                    display this help and exit\n");
        return;
    }

    while (fgets(buffer, LINE_BUFSIZE, stdin) != NULL) {
        printf("%s", buffer);

        for (int i = 0; i < file_count; i++) {
            fputs(buffer, files[i]);
        }
    }

    for (int i = 0; i < file_count; i++) {
        fclose(files[i]);
    }
}


void echo(char** args) {
    for(int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i + 1] != NULL) {
            printf(" ");
        }
    }
    printf("\n");
}

int handle_pipe(char* command) {
    char* commands[2];
    char* pipe_pos = strchr(command, '|');

    if (pipe_pos == NULL) {
        return 0;
    }

    *pipe_pos = '\0';
    commands[0] = command;
    commands[1] = pipe_pos + 1;

    while (*commands[1] == ' ') commands[1]++;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, RED "[sh]: pipe creation failed\n" RESET);
        return 0;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[PIPE_READ]);
        dup2(pipefd[PIPE_WRITE], STDOUT_FILENO);
        close(pipefd[PIPE_WRITE]);

        char** args = split_line(commands[0]);
        if (strcmp(args[0], "echo") == 0) {
            echo(args);
        } else if (strcmp(args[0], "tee") == 0) {
            tee(args);
        }
        free(args);
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[PIPE_WRITE]);
        dup2(pipefd[PIPE_READ], STDIN_FILENO);
        close(pipefd[PIPE_READ]);

        char** args = split_line(commands[1]);
        if (strcmp(args[0], "echo") == 0) {
            echo(args);
        } else if (strcmp(args[0], "tee") == 0) {
            tee(args);
        }
        free(args);
        exit(0);
    }

    close(pipefd[PIPE_READ]);
    close(pipefd[PIPE_WRITE]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 1;
}

void watch(char** args) {
    printf("watch\n");
}

int main(int argc, char *argv[], char *env[]) {
    char* username = login_handler();
    shell(username);
    free(username);
    return 0;
}