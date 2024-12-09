
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>


#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <inttypes.h>

#define MICROSECONDS_IN_MILLISECOND 1000
#define MAX_ARGS 2048
#define MAX_PROCESSES 100
#define MAX_COMMAND_LENGTH 2048
#define DEFAULT_BURST_TIME 1000

typedef struct
{

    char *command;
    char *command_hash;

    bool started;
    pid_t process_id;
    uint64_t arrival_time;
    uint64_t start_time;
    uint64_t completion_time;
    bool finished;
    bool error;
    uint64_t turnaround_time;
    uint64_t waiting_time;
    uint64_t response_time;
    uint64_t burst_time;
    uint64_t expected_burst_time;

} Process;

typedef struct
{
    Process **data;
    int front;
    int rear;
    int capacity;
    int current_size;
} Queue;


Queue *create_queue(int capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->data = (Process **)malloc(sizeof(Process *) * capacity);
    queue->capacity = capacity;
    queue->front = 0;
    queue->rear = -1;
    queue->current_size = 0;
    return queue;
}


bool is_empty(Queue *queue)
{
    return queue->current_size == 0;
}


int size(Queue *queue)
{
    return queue->current_size;
}


void push(Queue *queue, Process *process)
{
    if (queue->current_size == queue->capacity)
    {
        printf("Queue is full, cannot push more elements.\n");
        return;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->data[queue->rear] = process;
    queue->current_size++;
}


Process *pop(Queue *queue)
{
    if (is_empty(queue))
    {
        printf("Queue is empty, cannot pop elements.\n");
        return NULL;
    }
    Process *front_process = queue->data[queue->front];
    queue->data[queue->front] = NULL;
    queue->front = (queue->front + 1) % queue->capacity;
    queue->current_size--;
    return front_process;
}


void free_queue(Queue *queue)
{
    if (queue != NULL)
    {
        free(queue->data);
        free(queue);
    }
}

bool write_header(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
        return false;

    fprintf(file, "Command,Finished,Error,Burst Time,Turnaround Time,Waiting Time,Response Time\n");
    fclose(file);
    return true;
}

void show_args(char *args[], int max_num_args)
{
    for (int i = 0; i < max_num_args; i++)
    {
        if (args[i] == NULL)
            break;
        printf("args[%d]: $%s$\n", i, args[i]);
    }
}

void unhandled_exit(const char *message)
{
    fprintf(stderr, "%s\n", message);
    fprintf(stderr, "unhandled exit!\n");
    exit(EXIT_FAILURE);
}

void parse_command(char line[], char *args[])

{

    int n = strlen(line), ptr = 0, arg_id = 0;
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

        else
        {
            ptr_end++;
            while (ptr_end < n && (line[ptr_end] != '"'))
                ptr_end++;
            ptr_end++;
        }

        int arg_length = ptr_end - ptr;

        args[arg_id] = (char *)malloc(arg_length + 1);
        strncpy(args[arg_id], &line[ptr], arg_length);
        args[arg_id][arg_length] = '\0';

        arg_id++;
        ptr = ptr_end;
    }
    args[arg_id] = NULL;
}

bool describe_process(const Process *process, const char *filename)
{

  
    FILE *file = fopen(filename, "a");
    if (file == NULL)
        return false;

    // printf("Command [%s] finished\n", process->command);
    fprintf(file, "%s,%s,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
            process->command,
            (process->finished && (!process->error)) ? "Yes" : "No",
            process->error ? "Yes" : "No",
            process->burst_time,
            process->turnaround_time,
            process->waiting_time,
            process->response_time);

    fclose(file);
    return true;
}

uint64_t get_current_time_millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t milliseconds = (uint64_t)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
    return milliseconds;
}

void check_process_state(Process *process)
{
    pid_t pid = process->process_id;
    int status;
    // Use WUNTRACED to check if the child has stopped
    if (waitpid(pid, &status, WUNTRACED) > 0)
    {
        if (WIFEXITED(status))
        {
            process->completion_time = get_current_time_millis();
            process->finished = true;
            process->error = (WEXITSTATUS(status) != 0);
            process->turnaround_time = (process->completion_time - process->arrival_time);
            process->waiting_time = process->turnaround_time - process->burst_time;
        }
        else if (WIFSIGNALED(status))
        {
            unhandled_exit("Child process killed!");
        }
        else if (WIFSTOPPED(status))
        {
            // printf("Command [%s] paused\n\n", process->command);
        }
    }
    else
        unhandled_exit("waitpid failed!");
}

void show_context_switch(uint64_t context_start_time, uint64_t context_end_time, const char *command)
{
    // printf("%-50s | %06" PRIu64 " ms | %06" PRIu64 " ms\n", command, context_start_time, context_end_time);
    printf("%-50s | %06" PRIu64 " ms | %06" PRIu64 " ms\n", command, context_start_time, context_end_time);

}

void run_process_for_quantum(Process *process, uint64_t quantum, uint64_t scheduler_start_time)
{
    pid_t pid = process->process_id;
    uint64_t context_start_time = get_current_time_millis() - scheduler_start_time;
    if (kill(pid, SIGCONT))
        unhandled_exit("Process resuming failed!");
    usleep(quantum * MICROSECONDS_IN_MILLISECOND);
    if (kill(pid, SIGSTOP))
        unhandled_exit("Process stopping failed!");
    uint64_t context_end_time = get_current_time_millis() - scheduler_start_time;
    process->burst_time += (context_end_time - context_start_time);
    show_context_switch(context_start_time, context_end_time, process->command);
    check_process_state(process);
}

void init_process_run_for_quantum(Process *process, uint64_t quantum, bool to_completion, uint64_t scheduler_start_time)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        unhandled_exit("fork failed!");
    }

    else if (pid == 0) // child process
    {

        char *args[MAX_ARGS + 1];
        for (size_t i = 0; i < MAX_ARGS; i++)
            args[i] = NULL;

        parse_command(process->command, args);
        if (execvp(args[0], args) == -1)
        {

            exit(EXIT_FAILURE);
        }
    }
    else // parent process
    {

        process->start_time = get_current_time_millis();
        process->process_id = pid;
        process->started = true;
        process->response_time = (process->start_time - process->arrival_time);
        process->burst_time = 0;

        if (to_completion)
        {
            
            check_process_state(process);
            process->burst_time = (process->completion_time - process->start_time);
            uint64_t context_start_time = process->start_time - scheduler_start_time;
            uint64_t context_end_time = process->completion_time - scheduler_start_time;
            process->waiting_time = process->turnaround_time - process->burst_time;
            show_context_switch(context_start_time, context_end_time, process->command);

        }
        else
        {
            run_process_for_quantum(process, quantum, scheduler_start_time);
        }
    }
}

bool all_empty(Queue *queues[], int num_queues)
{
    for (int i = 0; i < num_queues; i++)
        if (!is_empty(queues[i]))
            return false;

    return true;
}

void boost_priorities(Queue *queues[], int num_queues)
{
    for (int i = 1; i < num_queues; i++)
        while (!is_empty(queues[i]))
        {
            Process *process = pop(queues[i]);
            push(queues[0], process);
        }
}

char *generate_command_hash(const char command[]) // remove spaces and concatenate 
{
    size_t char_cnt = 0;
    for (size_t i = 0; i < strlen(command); i++)
        if (command[i] != ' ')
            ++char_cnt;

    char *hash = (char *)malloc(char_cnt + 1);
    if (hash == NULL)
        unhandled_exit("Failed to allocate memory for command hash");

    size_t j = 0;
    for (size_t i = 0; command[i] != '\0'; i++)
        if (command[i] != ' ')
            hash[j++] = command[i];

    hash[j] = '\0';
    return hash;
}

Process *create_process(const char input[], char *command_hash, uint64_t expected_burst_time)
{
    Process *process = (Process *)malloc(sizeof(Process));
    if (process == NULL)
    {
        perror("Failed to allocate memory for Process");
        exit(EXIT_FAILURE);
    }

    process->command = (char *)malloc(strlen(input) + 1);
    if (process->command == NULL)
    {
        unhandled_exit("Failed to allocate memory for process command");
        free(process);
        exit(EXIT_FAILURE);
    }
    process->command = strdup(input);
    process->command_hash = command_hash;
    process->arrival_time = get_current_time_millis();
    process->started = false;
    process->finished = false;
    process->burst_time = 0;
    process->expected_burst_time = expected_burst_time;
    return process;
}

void set_nonblocking_io()
{
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1)
        unhandled_exit("Failed to set non-blocking IO");

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1)
        unhandled_exit("Failed to set non-blocking IO");
}

uint64_t get_average_burst_time(Process *processes[], size_t process_cnt, char *command_hash, bool *found, uint64_t default_burst)
{
    uint64_t match_burst_time_sum = 0, match_count = 0;
    for (size_t i = 0; i < process_cnt; i++)
        if ((processes[i]->finished) && (!processes[i]->error) && (strcmp(processes[i]->command_hash, command_hash) == 0))
        {
            match_burst_time_sum += processes[i]->burst_time;
            match_count++;
            (*found) = true;
        }

    return match_count == 0 ? default_burst : match_burst_time_sum / match_count;
}

void update_expected_burst_time(Process *processes[], size_t process_cnt, const char *command_hash, uint64_t new_burst_time)
{

    for (size_t i = 0; i < process_cnt; i++)
        if ((!processes[i]->finished) && (strcmp(processes[i]->command_hash, command_hash) == 0))
            processes[i]->expected_burst_time = new_burst_time;
}

uint64_t get_SJF_index(Process *processes[], size_t process_cnt)
{

    size_t min_index = process_cnt;
    uint64_t min_burst_time = UINT64_MAX;
    for (size_t i = 0; i < process_cnt; i++)
    {

        if (!processes[i]->finished && processes[i]->expected_burst_time < min_burst_time)
        {
            min_burst_time = processes[i]->expected_burst_time;
            min_index = i;
        }
    }

    return min_index;
}
