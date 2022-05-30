#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>

int main_pid;
int is_quit = 0;
char *prompt = "hello:";
char lastCommand[1024];



/* Command component linked list 
   (generated after seperating the whole command with pipe "|") */
typedef struct command_component {
  char *command[10];
  struct command_component *next;
} command_component;


/* To handle with Ctrl+C input */
void intHandler(int dummy) {
    strcpy(lastCommand, "^C");
    if (dummy == SIGTSTP){
        exit(0);
    }
    if (getpid() == main_pid){
        printf("\nYou typed Control-C!\n");
        // printf("%s: ", prompt);
    } 
}


/* Check if typed 'quit' */
void checkQuit(char* token){
    if (strcmp(token, "quit") == 0){
        kill(0, SIGTSTP);
    }
    if (strcmp(token, "^C") == 0){
        printf("You typed Control-C!\n");
    }
}

/* Check if typed '!!' */
char* checkLastCommand(char* token){
    if (strcmp(token, "!!") == 0){
        /* Execution of the last command */
        return lastCommand;
    }
    return token;
}


/* Check if typed 'prompt = newPrompt' */
void checkChangePromt(char* token1, char* token2, char* token3){
    if (strcmp(token1, "prompt") == 0 && strcmp(token2, "=") == 0){
        prompt = token3;
    }
}

/* Close both sides of fd pipe */
void close_pipe(int fd[2]) {
    close(fd[0]);
    close(fd[1]);
}

int main() {
    main_pid = getpid();
    signal(SIGINT, intHandler); // handel with Ctrl+c
    signal(SIGTSTP, intHandler); // handel with Ctrl+Z
    char command[1024];
    char *token;
    int i;
    char *outfile;
    int fd, amper, redirect, status, argc1;
    int pipe_one[2];
    int pipe_two[2];
    command_component *root;
    int pipes_num;
    while (1) {
        printf("%s ", prompt);
        ////////////////////////////////
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        strcpy(command, checkLastCommand(command));
        if (strlen(command) != 0)strcpy(lastCommand, command);
 
        /* parse command line */
        i = 0;
        pipes_num = 0;
        token = strtok(command, " ");
        root = (command_component*) malloc(sizeof(command_component));
        root->next = NULL;
        command_component *cur = root;
        while (token != NULL) {
            checkQuit(token);
            // printf("%s\n", token);
            cur->command[i] = token;
            token = strtok(NULL, " ");
            i++;
            if (token && !strcmp(token, "|")) {
                token = strtok(NULL, " "); // skip empty space after "|"
                pipes_num++;
                cur->command[i] = NULL;
                i = 0;
                command_component *next = (command_component*) malloc(sizeof(command_component));
                cur->next = next;
                cur = cur->next;
                cur->next = NULL;
                continue;
            }
        }
        cur->command[i] = NULL;
        argc1 = i;

        /* Is command empty */
        if (root->command[0] == NULL) continue;

        checkChangePromt(root->command[0], root->command[1], root->command[2]);

        /* Does command line end with & */
        if (!strcmp(cur->command[argc1 - 1], "&")) {
            amper = 1;
            root->command[argc1 - 1] = NULL;
        } else amper = 0;

        /* Handle redirection */
        if (argc1 > 1 && !strcmp(cur->command[argc1 - 2], ">")) {
            redirect = 1;
            root->command[argc1 - 2] = NULL;
            outfile = root->command[argc1 - 1];
        } else if (argc1 > 1 && !strcmp(cur->command[argc1 - 2], "2>")) {
            redirect = 2;
            root->command[argc1 - 2] = NULL;
            outfile = root->command[argc1 - 1];
        } else redirect = 0;

        /* for commands not part of the shell command language */
        if (fork() == 0) {
            /* redirection of IO ? */
            if (redirect == 1) { /* redirect stdout */
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            } else if (redirect == 2) { /* redirect stderr */
                fd = creat(outfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
            }
            if (pipes_num > 0) {
                cur = root;
                /* Create pipe (0 = pipe output. 1 = pipe input) */
                pipe(pipe_one);
                /* for more then 1 pipe we need another pipe for chaining */
                if (pipes_num > 1) pipe(pipe_two);
                /* Iterate over all command components except the last component */
                int pipes_iterator = pipes_num-1, pipe_switcher = 1, status = 1;
                /* Run first component (we need to get the input from the standart input) */
                pid_t pid = fork();
                if (pid == 0) {
                    //printf("waiting 1\n");
                    dup2(pipe_one[1], 1);
                    close(pipe_one[0]);
                    if (pipes_num > 1) close_pipe(pipe_two);
                    execvp(cur->command[0], cur->command);
                    exit(0);
                } else {
                    waitpid(pid, &status, 0);
                    close(pipe_one[1]);
                    cur = cur->next;
                }
                /* Iterate over the middle components (except first and last) */
                while (pipes_iterator > 0) {
                    pid = fork();
                    if (pid == 0) {
                        //printf("waiting 2\n");
                        if (pipe_switcher % 2 == 1) {
                            dup2(pipe_one[0], 0);
                            dup2(pipe_two[1], 1);
                        } else {
                            dup2(pipe_two[0], 0);
                            dup2(pipe_one[1], 1);
                        }
                        execvp(cur->command[0], cur->command);
                        exit(0);  
                    } else {
                        waitpid(pid, &status, 0);
                        if (pipe_switcher % 2 == 1) {
                            close(pipe_two[1]);
                            //close_pipe(pipe_one);
                            close(pipe_one[0]);
                            pipe(pipe_one);
                        } else {
                            close(pipe_one[1]);
                            //close_pipe(pipe_two);
                            close(pipe_two[0]);
                            pipe(pipe_two);
                        }
                        cur = cur->next;
                        pipe_switcher++;
                        pipes_iterator--;
                    }
                }
                /* Run last component (we need to output to the standart output) */
                pid = fork();
                if (pid == 0) {
                    //printf("waiting 3\n");
                    if (pipe_switcher % 2 == 0) dup2(pipe_two[0], 0);
                    else dup2(pipe_one[0], 0);
                    execvp(cur->command[0], cur->command);
                    exit(0);
                } else {
                    waitpid(pid, &status, 0);
                    close_pipe(pipe_one);
                    if (pipes_num > 1) close_pipe(pipe_two);
                }
            } else {
                execvp(root->command[0], root->command);
            }
        }

        /* Parent continues over here
           (waits for child to exit if required) */
        if (amper == 0) wait(&status); // retid = wait(&status);

        /* Free up all command components struct dynamic allocation */
        command_component *prev = root;
        cur = root;
        for (int i = 0; i <= pipes_num; i++) {
            prev = cur;
            cur = cur->next;
            free(prev);
        }
    }
}