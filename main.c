#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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

void shell(const char* username);
char* read_line(void);
char** split_line(char* line);
int my_system(char* const command);
void chmod(char** args);
void find(char** args);
void tee(char** args);
void watch(char** args);
int login(char* const user, char* const password);
char* login_handler(void);

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



int my_system(char * const command){
    char** args = split_line(command);

    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (strcmp(args[0], "chmod") == 0) {
            chmod(args);
        }else if(strcmp(args[0], "find") == 0) {
            find(args);
        }else if(strcmp(args[0], "tee") == 0) {
            tee(args);
        }else if(strcmp(args[0], "watch") == 0) {
            watch(args);
        }
    }else if (pid < 0) {
        fprintf(stderr, RED "[sh]: fork error\n" RESET);
        exit(EXIT_FORKING_FAILURE);
    }else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        }while (!WIFEXITED(status) && !WIFSIGNALED(status));
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

void chmod(char** args) {
    int i;

    // That's how you'll go through the args list
    for(i = 0 ; args[i] != NULL; i++) {
        printf("argumnet on index %d : %s\n", i, args[i]);
    }
}

void find(char** args) {
    printf("find\n");
}

void tee(char** args) {
    printf("tee\n");
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