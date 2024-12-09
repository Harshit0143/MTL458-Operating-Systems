#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 100
#define INPUT_FILENAME "input-part1.txt"
#define OUTPUT_FILENAME "output-part1.txt"
#define TERMINATING_INT 0

int buffer[BUFFER_SIZE];
int fill = 0;
int use = 0;
int count = 0;

pthread_cond_t empty_cond, fill_cond;
pthread_mutex_t mutex;

void exit_error(char *message)
{
    perror(message);
    exit(1);
}

void cpu_load(double burst_time)
{
    volatile double cpu_time_consumed = 0.0;
    while (cpu_time_consumed < burst_time)
        cpu_time_consumed = (double)clock() / CLOCKS_PER_SEC;
}

void put(int value)
{
    buffer[fill] = value;
    fill = (fill + 1) % BUFFER_SIZE;
    count++;
}

int get()
{
    int tmp = buffer[use];
    use = (use + 1) % BUFFER_SIZE;
    count--;
    return tmp;
}

void show_buffer(FILE *output_file, int consumed)
{
    fprintf(output_file, "Consumed:[%d],Buffer-State:[", consumed);
    int true_count = count;
    if (count > 0 && buffer[(use + count - 1) % BUFFER_SIZE] == TERMINATING_INT)
        --true_count;
    for (int i = 0; i < true_count; i++)
    {
        fprintf(output_file, "%d", buffer[(use + i) % BUFFER_SIZE]);
        if (i < true_count - 1)
            fprintf(output_file, ",");
    }
    fprintf(output_file, "]\n");
}

// Producer function
void *producer(void *arg)
{
    FILE *input_file = fopen(INPUT_FILENAME, "r");
    if (input_file == NULL)
        exit_error("Error opening input file");
    int value;
    while (1)
    {
        fscanf(input_file, "%d", &value);
        pthread_mutex_lock(&mutex);
        while (count == BUFFER_SIZE)
            pthread_cond_wait(&empty_cond, &mutex);
        put(value);
        pthread_cond_signal(&fill_cond);
        pthread_mutex_unlock(&mutex);
        if (value == TERMINATING_INT) // need to put it in buffer else consumer won't know when to stop
            break;
    }
    fclose(input_file);
    return NULL;
}

void *consumer(void *arg)
{
    FILE *output_file = fopen(OUTPUT_FILENAME, "w"); // TODO APPEND MODE?
    if (output_file == NULL)
        exit_error("Error opening output file");

    while (1)
    {
        pthread_mutex_lock(&mutex);
        while (count == 0)
            pthread_cond_wait(&fill_cond, &mutex);
        int tmp = get();
        if (tmp == TERMINATING_INT)
            break;
        /*
        at this point the producer thread has already exited and
        program has to end so we don't bother releasing the lock
        */
        show_buffer(output_file, tmp);
        pthread_cond_signal(&empty_cond);
        pthread_mutex_unlock(&mutex);
    }

    fclose(output_file);

    return NULL;
}

int main()
{
    pthread_t producer_thread, consumer_thread;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&empty_cond, NULL);
    pthread_cond_init(&fill_cond, NULL);

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    pthread_cond_destroy(&empty_cond);
    pthread_cond_destroy(&fill_cond);
    pthread_mutex_destroy(&mutex);
    return 0;
}
