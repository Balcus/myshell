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

char* read_line(void);
char** split_line(char* line);
int my_system(char* const command);
void chmod(char** args);
void find(char** args);
void tee(char** args);
void watch(char** args);
int login(char* const user, char* const password);

void shell(void) {
    char *line;
    int status;

    do {
        printf("> ");
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
        fprintf(stderr, "[sh]: allocation error\n");
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
                fprintf(stderr, "[sh]: allocation error\n");
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
        fprintf(stderr, "[sh]: allocation error\n");
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
                fprintf(stderr, "[sh]: allocation error\n");
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
        fprintf(stderr, "[sh]: fork error\n");
        exit(EXIT_FORKING_FAILURE);
    }else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        }while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    free(args);
    return 1;
}

int login(char * const user, char * const password){
   
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
    shell();
    return 0;
}