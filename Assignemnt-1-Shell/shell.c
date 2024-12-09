#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
// #include <fcntl.h> For STDOUT_FILENO O_CREAT, O_WRONLY, O_TRUNC S_IRWXU

#define MAX_LINE 2048
#define MAX_ARGS 2048
#define PATH_MAX 1024    // max length of directroy path
#define MAX_HISTORY 1000 // past 1000 commands in history
void show_error()
{
    fprintf(stderr, "Invalid Command\n");
}
void exit_program(char *args[], char *history[], int exit_code)
{
    for (int i = 0; i < MAX_ARGS + 1; i++)
        if (args[i] != NULL)
            free(args[i]);

    for (int i = 0; i < MAX_HISTORY; i++)
        if (history[i] != NULL)
            free(history[i]);
    exit(exit_code);
}
int parse_command(char line[], char *args[], int *output_file_arg_id)
/*
    two pointers to split command by spaces and store in args

    line: raw input string from user
    args: array of strings, line split by sapces
    output_file_arg_id: stores the index of the argument which is the output file, in case of redirection, else -1
    return: pipe_id, index of the pipe character in the args array, in case of piping, -1 if no pipe
*/

{
    (*output_file_arg_id) = -1;
    int n = strlen(line), ptr = 0, arg_id = 0, pipe_id = -1;
    while (ptr < n)
    {
        while (ptr < n && line[ptr] == ' ')
            ptr++;
        if (ptr == n)
            break;

        int ptr_end = ptr;

        if (line[ptr] != '"')
        {
            while (ptr_end < n && !(line[ptr_end] == ' '))
                ptr_end++;
        }

        else // if in quotes ignote spaces and split by quotes
        {
            ptr_end++;
            while (ptr_end < n && (line[ptr_end] != '"'))
                ptr_end++;
            ptr_end++;
        }

        int arg_length = ptr_end - ptr;

        if (args[arg_id] != NULL)
            free(args[arg_id]);

        if (arg_length == 1 && line[ptr] == '|') // pipe
        {
            pipe_id = arg_id;
            args[arg_id] = NULL;
        }
        else if (arg_length == 1 && line[ptr] == '>') // output redirection
        {
            (*output_file_arg_id) = arg_id + 1;
            args[arg_id] = NULL;
        }

        else
        {
            args[arg_id] = (char *)malloc(arg_length + 1);
            strncpy(args[arg_id], &line[ptr], arg_length);
            args[arg_id][arg_length] = '\0';
        }
        arg_id++;
        ptr = ptr_end;
    }
    args[arg_id] = NULL; // denotes end of args
    return pipe_id;
}

void execute_cd(char *args[], char prev_dir[])
{
    char curr_dir[PATH_MAX];

    if (getcwd(curr_dir, PATH_MAX) == NULL) // get current working directory
    {
        show_error();
        return;
    }
    char *home_dir = getenv("HOME");
    if (args[1] == NULL || strcmp(args[1], "~") == 0) // cd or cd ~
    {
        if (chdir(home_dir) == -1)
        {
            show_error();
            return;
        }
    }

    else if (strcmp(args[1], "-") == 0) // cd -
    {
        if (strlen(prev_dir) > 0)
        {
            printf("%s\n", prev_dir);
            if (chdir(prev_dir) == -1)
            {
                show_error();
                return;
            }
        }
        else
        {
            show_error();
            return;
        }
    }
    else if (chdir(args[1]) == -1) // cd <dir>
    {
        show_error();
        return;
    }
    strcpy(prev_dir, curr_dir); // update previous directory when cd is successful
}
void execute_history(char *history[], int command_count) // past MAX_HISTORY commands stored in history cyclically
{

    int start = command_count >= MAX_HISTORY ? command_count - MAX_HISTORY : 0;
    int end = command_count - 1;
    for (int i = start; i <= end; i++)
        printf("%s\n", history[i % MAX_HISTORY]);
}

int handle_builtin_commands(char *args[], char prev_dir[], char *history[], int command_count, int pipe_id)
// handlels: exit cd history
{
    if (strcmp(args[0], "exit") == 0)
        exit_program(args, history, 0); // frees up dynamically allocated memory and exits woth exit status 0

    else if (strcmp(args[0], "cd") == 0)
    {
        execute_cd(args, prev_dir);
        return 1;
    }
    else if (strcmp(args[0], "history") == 0 && pipe_id == -1)
    {
        execute_history(history, command_count);
        return 1;
    }

    return 0; // nothing was executed. not a builtin command
}

void execute_single_command(char *args[], int output_file_arg_id)
// output_file_arg_id: index of the argument which is the output file, in case of redirection, else -1

{

    int rc = fork();
    if (rc < 0)
        show_error();
    else if (rc == 0)
    {
        // if (output_file_arg_id != -1)
        // {
        //     close(STDOUT_FILENO);
        //     open(args[output_file_arg_id], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
        // }
        if (execvp(args[0], args) == -1)
            show_error();
        exit(1);
    }
    else
        wait(NULL);
}
void execute_pipe_command(char *args[], int pipe_id, char *history[], int command_count, int output_file_arg_id)
// output_file_arg_id: index of the argument which is the output file, in case of redirection, else -1

{

    int rc = fork();
    if (rc < 0)
        show_error();

    else if (rc == 0) // child processes (total two)
    {
        int fd[2];
        pipe(fd);
        int rc2 = fork(); // second child process for first command in pipe

        if (rc2 < 0)
            show_error();

        else if (rc2 == 0)
        {
            close(fd[0]);               // close read end of pipe
            dup2(fd[1], STDOUT_FILENO); // direct stdout to write end of pipe
            close(fd[1]);               // close write end of pipe
            if (strcmp(args[0], "history") == 0)
            {
                execute_history(history, command_count);
                exit(0);
            }
            if (execvp(args[0], args) == -1) // first command in pipe
                show_error();
            exit(1);
        }
        else // first child process for second command in pipe
        {
            wait(NULL);                // wait till first command in pipe is finished
            close(fd[1]);              // close write end of pipe
            dup2(fd[0], STDIN_FILENO); // direct stdin to read end of pipe
            close(fd[0]);              // close read end of pipe
            // if (output_file_arg_id != -1)
            // {
            //     close(STDOUT_FILENO);
            //     open(args[output_file_arg_id], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
            // }
            if (execvp(args[pipe_id + 1], &args[pipe_id + 1]) == -1) // second command in pipe
                show_error();
            exit(0);
        }
    }
    else
        wait(NULL);
}
void show_args(char *args[]) // for debugging
{
    for (int i = 0; i < MAX_ARGS; i++)
    {
        if (args[i] == NULL)
            break;
        printf("args[%d]: $%s$\n", i, args[i]);
    }
}
void show_args_pipe(char *args[], int pipe_id) // for debugging
{
    int i = 0;
    printf("First   command:\n");
    while (i < MAX_ARGS)
    {
        if (args[i] == NULL)
            break;
        printf("args[%d]: $%s$\n", i, args[i]);
        i++;
    }
    i++;
    printf("Second command\n");
    while (i < MAX_ARGS)
    {
        if (args[i] == NULL)
            break;
        printf("args[%d]: $%s$\n", i, args[i]);
        i++;
    }
}

void set_null(char *history[], char *args[], char prev_dir[]) // initialize all strings to NULL
{
    prev_dir[0] = '\0';
    for (int i = 0; i < MAX_ARGS + 1; i++)
        args[i] = NULL;
    for (int i = 0; i < MAX_HISTORY; i++)
        history[i] = NULL;
}
void add_history(char *history[], char line[], int *command_count)
{
    int str_locn = (*command_count) % MAX_HISTORY;
    line[strcspn(line, "\n")] = '\0'; // remove trailing newline

    if (history[str_locn] != NULL)
        free(history[str_locn]);
    history[str_locn] = strdup(line);

    (*command_count)++; // total number of commands executed
}

int main()
{
    int command_count = 0;
    char line[MAX_LINE + 1], *history[MAX_HISTORY], *args[MAX_ARGS + 1];
    char prev_dir[PATH_MAX];
    int output_file_arg_id = -1;
    set_null(history, args, prev_dir);
    while (1)
    {
        printf("MTL458 > ");

        if (fgets(line, MAX_LINE, stdin) == NULL)
            continue;

        if (strlen(line) == 1 && line[0] == '\n')
            continue;

        add_history(history, line, &command_count);

        int pipe_id = parse_command(line, args, &output_file_arg_id);

        // if (output_file_arg_id != -1)
        //     printf("output_file: %s\n", args[output_file_arg_id]);

        if (handle_builtin_commands(args, prev_dir, history, command_count, pipe_id))
            continue;

        if (pipe_id == -1)
        {
            // show_args(args);
            execute_single_command(args, output_file_arg_id);
        }
        else
        {
            // show_args_pipe(args, pipe_id);
            execute_pipe_command(args, pipe_id, history, command_count, output_file_arg_id);
        }
    }
    return 0;
}
