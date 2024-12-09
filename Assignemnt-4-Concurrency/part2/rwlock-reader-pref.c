#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "zemaphore.h" // TODO: change to <semaphore.h>

#define OUTPUT_FILENAME "output-reader-pref.txt"
#define SHARED_FILENAME "shared-file.txt"
#define WRITE_LOG_MESSAGE "Writing,Number-of-readers-present:[%d]\n"
#define READ_LOG_MESSAGE "Reading,Number-of-readers-present:[%d]\n"
#define WRITE_MESSAGE "Hello world!\n"
#define IO_ERROR_MESSAGE "Error opening file"
// TODO lock needed while logging?
void exit_error(char *message)
{
    perror(message);
    exit(1);
}
typedef struct _rwlock_t
{
    sem_t lock;
    sem_t writelock;
    sem_t log_lock;
    int readers;
} rwlock_t;

void rwlock_init(rwlock_t *rw)
{
    rw->readers = 0;
    sem_init(&rw->lock, 0, 1);
    sem_init(&rw->writelock, 0, 1);
    sem_init(&rw->log_lock, 0, 1);
}

void rwlock_acquire_readlock(rwlock_t *rw)
{
    sem_wait(&rw->lock);
    if (rw->readers == 0)
        sem_wait(&rw->writelock);
    rw->readers++; //increases after accquired else buggy
    sem_post(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw)
{
    sem_wait(&rw->lock);
    if (rw->readers == 1)
        sem_post(&rw->writelock);
    rw->readers--;
    sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw)
{
    sem_wait(&rw->writelock);
}

void rwlock_release_writelock(rwlock_t *rw)
{
    sem_post(&rw->writelock);
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
        // TODO what to do here?
        char line[256];
        while (fgets(line, sizeof(line), shared_file) != NULL)
        {
            // TODO what to do here?
        }
        fclose(shared_file);
    }
    else
        exit_error(IO_ERROR_MESSAGE);

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
        exit_error(IO_ERROR_MESSAGE);

    rwlock_release_writelock(&rwlock);
    return NULL;
}

int main(int argc, char **argv)
{

    rwlock_init(&rwlock);
    if (argc != 3)
        return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    for (int i = 0; i < n; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (int i = 0; i < m; i++)
        pthread_create(&writers[i], NULL, writer, NULL);
        
    

    for (int i = 0; i < n; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++)
        pthread_join(writers[i], NULL);

    return 0;
}