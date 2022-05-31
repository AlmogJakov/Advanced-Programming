#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

int main_pid;
int is_quit = 0;
char *prompt;
char lastCommand[1024];
int es = 0;
int errInCd = 0;

typedef struct key_value {
   char* key;
   char* value;
   struct key_value *next;
} key_value;

key_value *key_value_root;

void key_value_add(char* key, char* value) {
    key_value *iter = key_value_root;
    while (iter->next != NULL) {
        iter = iter->next;
        if (strcmp(iter->key,key) == 0) {
            char* new_val = malloc(strlen(value) + 1); 
            strcpy(new_val, value);
            iter->value = new_val;
            return;
        }
    }
    key_value *next = (key_value*) malloc(sizeof(key_value));
    char* new_key = malloc(strlen(key) + 1); 
    strcpy(new_key, key);
    next->key = new_key;
    char* new_val = malloc(strlen(value) + 1); 
    strcpy(new_val, value);
    next->value = new_val;
    next->next = NULL;
    iter->next = next;
    return;
}

char* value_get(char* key) {
    key_value *iter = key_value_root;
    while (iter->next != NULL) {
        iter = iter->next;
        if (strcmp(iter->key,key) == 0) {
            return iter->value;
        }
    }
    return NULL;
}

/* Command component linked list
   (generated after seperating the whole command with pipe "|") */
typedef struct command_component {
    char *command[10];
    struct command_component *next;
} command_component;

// https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

/* To handle with Ctrl+C input */
void intHandler(int dummy) {
    strcpy(lastCommand, "^C");
    if (dummy == SIGTSTP) {
        exit(0);
    }
    if (getpid() == main_pid) {
        printf("\nYou typed Control-C!\n");
        char message[2] = " ";
        write(STDIN_FILENO, prompt, strlen(prompt)+1);
        write(STDIN_FILENO, message, strlen(message)+1);
    }
}

/* Check if typed 'quit' */
void checkQuit(char *token)
{
    if (strcmp(token, "quit") == 0){
        kill(0, SIGTSTP);
    }
    if (strcmp(token, "^C") == 0){
        printf("You typed Control-C!\n");
    }
}

/* Check if typed '!!' */
char *checkLastCommand(char *token)
{
    if (strcmp(token, "!!") == 0){
        /* Execution of the last command */
        return lastCommand;
    }
    return token;
}

/* Check if typed 'prompt = newPrompt' */
void checkChangePromt(char *token1, char *token2, char *token3) {
    if (strcmp(token1, "prompt") == 0 && strcmp(token2, "=") == 0) {
        //free(prompt);
        //prompt = malloc(strlen(token3) + 1);
        strcpy(prompt, token3);
    }
}


/* Check if typed 'echo $?' */
void checkExitStatus(command_component *list, int status) {
    while (list != NULL){
        for (int i = 0; i < 9; i++){
            if (list->command[i+1] == NULL){break;}
            if (strcmp(list->command[i], "echo") == 0 && strcmp(list->command[i+1], "$?") == 0) {
                if (!errInCd && WIFEXITED(status)) {
                    es = WEXITSTATUS(status);
                }
                else if (errInCd) {
                    es = 1;
                    errInCd = 0;
                }
                sprintf(list->command[i+1], "%d", es);
            }
        }
        list = list->next;
    }
}

/* Check if typed 'cd' */
int checkCdCommand(char *token1, char *token2) {
    if (!strcmp(token1, "cd")) {
        if (chdir(token2)){
            printf("cd: %s: No such file or directory\n", token2);
            errInCd = 1;
        }
        return 1;
    }
    return 0;
}


/* Close both sides of fd pipe */
void close_pipe(int fd[2]){
    close(fd[0]);
    close(fd[1]);
}

int main(){
    char *original_prompt = "hello:";
    /* Share the prompt name since we want to update the parent after the child changing the name */
    prompt = (char*)mmap(NULL, sizeof(char)*100, 0x1|0x2 , 0x01 | 0x20, -1, 0);
    /* same as:
        prompt = (char*)mmap(NULL, sizeof(char)*100, PROT_READ|PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS, -1, 0) */
    strcpy(prompt, original_prompt);
    main_pid = getpid();
    signal(SIGINT, intHandler);  // handel with Ctrl+c
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
    key_value_root = (key_value*) malloc(sizeof(key_value));
    key_value_root->next = NULL;
    while (1) {
        printf("%s ", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        /* Update the lest command */
        strcpy(command, str_replace(command, "!!", lastCommand));
        if (strlen(command) != 0)
            strcpy(lastCommand, command);

        i = 0;
        pipes_num = 0;
        token = strtok(command, " ");
        root = (command_component *)malloc(sizeof(command_component));
        root->next = NULL;
        command_component *cur = root;
        while (token != NULL){
            checkQuit(token);
            cur->command[i] = token;
            token = strtok(NULL, " ");
            i++;
            if (token && !strcmp(token, "|")){
                token = strtok(NULL, " "); // skip empty space after "|"
                pipes_num++;
                cur->command[i] = NULL;
                i = 0;
                command_component *next = (command_component *)malloc(sizeof(command_component));
                cur->next = next;
                cur = cur->next;
                cur->next = NULL;
                continue;
            }
        }

        cur->command[i] = NULL;
        argc1 = i;

        checkChangePromt(root->command[0], root->command[1], root->command[2]);
        checkExitStatus(root, status);
        if (checkCdCommand(root->command[0], root->command[1])){
            continue;
        }

        /* Handle named variables */
        command_component *iter = root;
        int num = pipes_num + 1;
        while (num > 0) {
            if (iter->command[0] == NULL){
                num--;
                iter = iter->next;
                continue;
            }
            if (iter->command[0][0] == '$' && strcmp(iter->command[1],"=") == 0) {
                key_value_add(iter->command[0]+1, iter->command[2]);
            }
            num--;
            iter = iter->next;
        }
        iter = root;
        num = pipes_num + 1;
        while (num > 0) {
            if (iter->command[0] == NULL) {
                num--;
                iter = iter->next;
                continue;
            }
            if (!(iter->command[0][0] == '$' && strcmp(iter->command[1],"=") == 0)) {
                for (int i = 0; i < 10; i++) {
                    if (iter->command[i] != NULL && iter->command[i][0] == '$') {
                        char *val = value_get(iter->command[i]+1);
                        if (val!=NULL) iter->command[i] = val;
                        else iter->command[i] = ""; // if the key doesnt found
                    }}}
            num--;
            iter = iter->next;
        }
        if (root->command[0] != NULL && root->command[1] != NULL && strcmp(root->command[0],"read") == 0) {
            char *key = root->command[1];
            char value[20];
            fgets(value, 20, stdin);
            value[strlen(value)-1] = '\0'; // remove new line ('\n')
            char *val = value;
            key_value_add(key, val);
            //continue;
        }

        /* Is command empty */
        if (root->command[0] == NULL)
            continue;

        /* Does command line end with & */
        if (!strcmp(cur->command[argc1 - 1], "&")){
            amper = 1;
            root->command[argc1 - 1] = NULL;
        }
        else
            amper = 0;

        /* Handle redirection */
        if (argc1 > 1 && !strcmp(cur->command[argc1 - 2], ">" )){
            redirect = 1;
            root->command[argc1 - 2] = NULL;
            outfile = root->command[argc1 - 1];
        }
        else if (argc1 > 1 && !strcmp(cur->command[argc1 - 2], ">>" )){
            redirect = 3;
            root->command[argc1 - 2] = NULL;
            outfile = root->command[argc1 - 1];
        }
        else if (argc1 > 1 && !strcmp(cur->command[argc1 - 2], "2>")){
            redirect = 2;
            root->command[argc1 - 2] = NULL;
            outfile = root->command[argc1 - 1];
        }
        else
            redirect = 0;

        /* for commands not part of the shell command language */
        if (fork() == 0) {
            /* redirection of IO ? */
            if (redirect == 1)
            { /* redirect stdout */
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            if (redirect == 3)
            { /* redirect stdout */
                fd = open(outfile, O_CREAT | O_WRONLY | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
            }
            else if (redirect == 2)
            { /* redirect stderr */
                fd = creat(outfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
            }
            if (pipes_num > 0){
                cur = root;
                /* Create pipe (0 = pipe output. 1 = pipe input) */
                pipe(pipe_one);
                /* for more then 1 pipe we need another pipe for chaining */
                if (pipes_num > 1)
                    pipe(pipe_two);
                /* For Iterating over all command components except the first & last components */
                int pipes_iterator = pipes_num - 1, pipe_switcher = 1, status = 1;
                /* Run first component (we need to get the input from the standart input) */
                pid_t pid = fork();
                if (pid == 0){
                    dup2(pipe_one[1], 1);
                    close(pipe_one[0]);
                    if (pipes_num > 1)
                        close_pipe(pipe_two);
                    execvp(cur->command[0], cur->command);
                    exit(0);
                } else {
                    waitpid(pid, &status, 0);
                    close(pipe_one[1]);
                    cur = cur->next;
                }
                /* Run [Iterate over] middle components (except first read name | echo heyand last)
                   while getting input from one pipe and output to other pipe */
                while (pipes_iterator > 0){
                    pid = fork();
                    if (pid == 0){
                        if (pipe_switcher % 2 == 1){
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
                            close(pipe_one[0]);
                            pipe(pipe_one);
                        } else {
                            close(pipe_one[1]);
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
                    if (pipe_switcher % 2 == 0)
                        dup2(pipe_two[0], 0);
                    else
                        dup2(pipe_one[0], 0);
                    execvp(cur->command[0], cur->command);
                    exit(0);
                } else {
                    waitpid(pid, &status, 0);
                    close_pipe(pipe_one);
                    if (pipes_num > 1)
                        close_pipe(pipe_two);
                }
            } else {
                execvp(root->command[0], root->command);
            }
        }

        /* Parent continues over here
           (waits for child to exit if required) */
        if (amper == 0)
            wait(&status); // retid = wait(&status);


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