#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#define MAGIC_ADDR 13243297
#define INIT_BYTES 4096
#define LOG2_INIT_BYTES 12

/*
This software has been tests to work on a 64-bit machine (i.e. size_t is 8 bytes)
We use int* magic and not int magic to ensire sizeof(header_t) == sizeof(node_t)
Otherwise, allored spae of 9 byted (8 byte header_t + 1 byte user) can't be converted to node_t on freeing
*/
typedef struct __header_t
{
    size_t size;
    int *magic;
} header_t;

typedef struct __node_t
{
    size_t size;
    struct __node_t *next;
} node_t;

static node_t *head = NULL;
static void *memory_start = NULL; // easir to debug

// first time heap memory request
void initialize_heap()
{
    if (head == NULL)
    {
        head = (node_t *)mmap(NULL, INIT_BYTES, PROT_READ | PROT_WRITE,
                              MAP_ANON | MAP_PRIVATE, -1, 0);
        if (head == MAP_FAILED)
        {
            return;
        }
        head->size = INIT_BYTES - sizeof(node_t);
        head->next = NULL;
        memory_start = head;
    }
}

// subsequent heap memory requests
node_t *request_heap_memory(size_t search_size)
{

    /*
        heap memory is requested in multiples of INIT_BYTES
        smallest multiple of INIT_BYTES strictly greater than search_size
    */
    size_t extra_size = ((search_size >> LOG2_INIT_BYTES) + 1) << LOG2_INIT_BYTES;
    node_t *ptr = (node_t *)mmap(NULL, extra_size, PROT_READ | PROT_WRITE,
                                 MAP_ANON | MAP_PRIVATE, -1, 0);
    if (ptr == (void *)-1)
        return NULL;

    ptr->size = extra_size - sizeof(node_t);
    ptr->next = NULL;
    return ptr;
}

// debugging
void show_node(node_t *node)
{
    printf("curr: %lu\n", (unsigned long)node - (unsigned long)memory_start);
    printf("size: %zu\n", node->size);
    if (node->next == NULL)
        printf("next: NULL\n");
    else
        printf("next: %lu\n", (unsigned long)node->next - (unsigned long)memory_start);
}
// for debuging
void show_free_list()
{
    printf("\nprinting free list\n");
    node_t *curr = head;
    while (curr != NULL)
    {
        show_node(curr);
        curr = curr->next;
    }
    printf("finished free list\n\n");
}

// First fit algorithm
node_t *search_free_list(size_t search_size, bool *found)
{
    node_t *prev = NULL;
    node_t *curr = head;
    while (curr != NULL)
    {
        if (curr->size >= search_size)
        {
            *found = true;
            return prev;
        }
        prev = curr;
        curr = curr->next;
    }
    return NULL;
}
// for coallescing. returns address to end of a free list node
void *get_next_block_ptr(node_t *ptr)
{
    return (void *)ptr + ptr->size + sizeof(node_t);
}

// coalescing the free list head
void coalesce_free_list()
{

    void *next_block_ptr = get_next_block_ptr(head);
    node_t *curr = head;
    node_t *prev = NULL;
    node_t *found_node_prev = NULL;
    node_t *found_node = head;
    while (curr != NULL)
    {
        // A B C --> merge A and B (B is the head in this case)
        if ((void *)curr == next_block_ptr)
        {

            found_node_prev = prev == head ? NULL : prev;
            // assert(prev != NULL);
            prev->next = head;
            head->size += curr->size + sizeof(node_t);
            node_t *new_head = head->next;
            head->next = curr->next;
            head = new_head;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    curr = head;
    // A B C --> merge B and C (B is the head in this case)
    while (curr != NULL)
    {
        if (get_next_block_ptr(curr) == (void *)found_node)
        {
            // assert(curr != found_node);
            curr->size += found_node->size + sizeof(node_t);
            if (found_node_prev == NULL)
                head = head->next;
            else
                found_node_prev->next = found_node->next;
            break;
        }
        curr = curr->next;
    }
}

void *my_malloc(size_t req_size)
{

    initialize_heap();
    if (head == NULL || req_size == 0)
        return NULL;

    size_t search_size = req_size + sizeof(header_t);
    bool found = false;
    node_t *prev = search_free_list(search_size, &found);
    
    if (!found) // request more heap sapce
    {
        node_t *new_node = request_heap_memory(search_size + sizeof(node_t));
        if (new_node == NULL)
            return NULL;
        new_node->next = head;
        coalesce_free_list();

        head = new_node;
        prev = NULL;
    }
    node_t *curr = (prev == NULL) ? head : prev->next;

    // split the free list node
    node_t *split_free = (node_t *)((void *)curr + search_size);
    size_t curr_size = curr->size; // split_free could be overwriting curr when search_size <= size_of(cirr)
    node_t *curr_next = curr->next;
    split_free->size = curr_size - search_size;
    split_free->next = curr_next;
    
    if (prev == NULL)
        head = split_free;
    else
        prev->next = split_free;
    /*
    When search_size exactly matches curr->size, split_free->size will be 0
    In that case the free list node will have size 0
    We don't handle it explicity, wait for it to coalese later
    */

    header_t *hptr = (header_t *)curr;
    hptr->size = req_size;
    hptr->magic = (void *)MAGIC_ADDR;

    // ptr to usable memory
    return (void *)hptr + sizeof(header_t);
}

void *my_calloc(size_t nelem, size_t size)
{
    void *ptr = my_malloc(nelem * size);
    if (ptr) // set memory to 0
        memset(ptr, 0, nelem * size);

    return ptr;
}

void my_free(void *ptr)
{

    if (ptr == NULL)
        return;
    header_t *hptr = (header_t *)(ptr - sizeof(header_t));
    if ((unsigned long)hptr->magic != MAGIC_ADDR)
    {
        fprintf(stderr, "Invalid free\n");
        return;
    }
    hptr->magic = NULL;
    /*
        * Handles double free
        * Handles freeing up memory in beteeen i.e.
            int *arr = my_malloc(8 * sizeof(int));
            my_free(arr + 2);
        But it's easy to hack it by filling up the memory with magic number
        But not possible to hack the double free case
    */
    node_t *new_node = (node_t *)hptr;

    /*
    Add freed node to free list at head.
    The coalesce_free_list() will merge the head at the right position, if possible
    */
    new_node->size = hptr->size + sizeof(header_t) - sizeof(node_t);
    new_node->next = head;
    head = new_node;
    coalesce_free_list();
    // show_free_list();
}
