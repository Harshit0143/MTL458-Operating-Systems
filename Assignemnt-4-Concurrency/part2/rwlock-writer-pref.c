// #include <semaphore.h>
#include <pthread.h>
#include <math.h>
#include "zemaphore.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define OUTPUT_FILENAME "output-writer-pref.txt"
#define SHARED_FILENAME "shared-file.txt"
#define WRITE_LOG_MESSAGE "Writing, Number of readers present: [%d]\n"
#define READ_LOG_MESSAGE "Reading, Number of readers present: [%d]\n"
#define WRITE_MESSAGE "Hello world!\n"
#define IO_ERROR_MESSAGE "Error opening file!\n"
// TODO: Iterate char by char
// TODO: Fix zemaphore.h, check message to log woth assignment statement
// TODO: implement cond_t and sigmal using semaohore
// TODO: implement mutex using semaphore
// TODO: number if readers and writers updated correctly?
void exit_error(const char *message)
{
    perror(message);
    exit(1);
}

void heavyLoadSimulation()
{
    volatile double result = 0.0;
    for (int i = 0; i < 1e2; ++i)
        result += sin(i) * cos(i);
}

typedef struct _rwlock_t
{
    pthread_mutex_t lock;
    pthread_cond_t ok_to_read;
    sem_t resource;
    sem_t log_lock;      // Protects access to log file
    int readers;         // Number of readers currently reading
    int writers_waiting; // Number of writers currently waiting
} rwlock_t;

void rwlock_init(rwlock_t *rw)
{
    pthread_mutex_init(&rw->lock, NULL);
    pthread_cond_init(&rw->ok_to_read, NULL);
    sem_init(&rw->resource, 0, 1);
    sem_init(&rw->log_lock, 0, 1);
    rw->readers = 0;
    rw->writers_waiting = 0;
}

void rwlock_acquire_readlock(rwlock_t *rw)
{

    pthread_mutex_lock(&rw->lock);
    while (rw->writers_waiting > 0)
        pthread_cond_wait(&rw->ok_to_read, &rw->lock);
    if (rw->readers == 0)
        sem_wait(&rw->resource);
    rw->readers++;
    pthread_mutex_unlock(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw)
{

    pthread_mutex_lock(&rw->lock);
    if (rw->readers == 1)
    {

        sem_post(&rw->resource);
        if (rw->writers_waiting == 0)
            pthread_cond_signal(&rw->ok_to_read);
    }
    rw->readers--;
    pthread_mutex_unlock(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw)
{
    pthread_mutex_lock(&rw->lock);
    rw->writers_waiting++;
    pthread_mutex_unlock(&rw->lock);
    sem_wait(&rw->resource);
    pthread_mutex_lock(&rw->lock);
    rw->writers_waiting--;
    pthread_mutex_unlock(&rw->lock);
}

void rwlock_release_writelock(rwlock_t *rw)
{
    sem_post(&rw->resource);
    pthread_mutex_lock(&rw->lock);
    if (rw->writers_waiting == 0)
        pthread_cond_signal(&rw->ok_to_read);
    pthread_mutex_unlock(&rw->lock);
}

rwlock_t rwlock;

void *reader(void *arg)
{
    rwlock_acquire_readlock(&rwlock);
    sem_wait(&rwlock.log_lock);
    FILE *logfile = fopen(OUTPUT_FILENAME, "a");
    if (logfile == NULL)
        exit_error(IO_ERROR_MESSAGE);
    fprintf(logfile, READ_LOG_MESSAGE, rwlock.readers);
    fclose(logfile);
    sem_post(&rwlock.log_lock);

    FILE *shared_file = fopen(SHARED_FILENAME, "r");
    if (shared_file)
    {
        char line[256];
        while (fgets(line, sizeof(line), shared_file) != NULL)
        {
            heavyLoadSimulation();
        }
        fclose(shared_file);
    }
    else
    {
        exit_error(IO_ERROR_MESSAGE);
    }

    rwlock_release_readlock(&rwlock);
    return NULL;
}

void *writer(void *arg)
{
    rwlock_acquire_writelock(&rwlock);
    sem_wait(&rwlock.log_lock);
    FILE *logfile = fopen(OUTPUT_FILENAME, "a");
    if (logfile == NULL)
        exit_error(IO_ERROR_MESSAGE);
    fprintf(logfile, WRITE_LOG_MESSAGE, rwlock.readers);
    fclose(logfile);
    sem_post(&rwlock.log_lock);
    FILE *shared_file = fopen(SHARED_FILENAME, "a");
    if (shared_file)
    {
        fprintf(shared_file, WRITE_MESSAGE);
        fclose(shared_file);
    }
    else
    {
        exit_error(IO_ERROR_MESSAGE);
    }

    rwlock_release_writelock(&rwlock);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    rwlock_init(&rwlock);

    for (int i = 0; i < n; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (int i = 0; i < m; i++)
        pthread_create(&writers[i], NULL, writer, NULL);


    
    // for (int i = 0; i < n; i++)
    // {
    //     pthread_create(&readers[i], NULL, reader, NULL);
    //     pthread_create(&writers[i], NULL, writer, NULL);
    // }

    for (int i = 0; i < n; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++)
        pthread_join(writers[i], NULL);

    return 0;
}
