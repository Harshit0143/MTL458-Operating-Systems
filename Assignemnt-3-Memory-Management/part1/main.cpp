#include <iostream>
#include <iomanip>
#define LOG2_KB_IN_MB 10
#define LOG2_B_IN_KB 10
#define MAX_PAGES 4194304 // (S_MAX * 1024) / P_MIN
#define MAX_N 1000000 // N_MAX
#define MAX_CACHE 1024 // K_MAX

using namespace std;

void exit_error(const string &message)
{
    cout << "error: " << message << '\n';
    exit(1);
}
/*
max-capacity circular queue
the data array is supplied by the user to to avoid K_MAX size memory allocation in each test case
so time complexity per test case is still O(used size) and not O(K_MAX)
*/
class Queue
{
private:
    int *_data;
    int _front, _rear, _capacity, _size;

public:
    Queue(int max_size, int *queue_arr)

    {
        _capacity = max_size;
        _data = queue_arr;
        _front = 0;
        _rear = -1;
        _size = 0;
    }

    void push(int item)
    {
        if (_size == _capacity)
            exit_error("queue is full");
        _rear = (_rear + 1) % _capacity;
        _data[_rear] = item;
        ++_size;
    }

    int pop()
    {
        if (_size == 0)
            exit_error("pop() from empty queue");

        int val = _data[_front];
        _front = (_front + 1) % _capacity;
        --_size;
        return val;
    }

    int is_full()
    {
        return _size == _capacity;
    }
};


/*
max-capacity stack.
the data array is supplied by the user to to avoid K_MAX size memory allocation in each test case
so time complexity per test case is still O(used size) and not O(K_MAX)
*/
class Stack
{
private:
    int *_data;
    int _top, _capacity;

    inline int size()
    {
        return _top + 1;
    }

public:
    Stack(int max_size, int *stack_arr)

    {
        _capacity = max_size;
        _data = stack_arr;

        _top = -1;
    }

    void push(int item)
    {
        if (size() == _capacity)
            exit_error("stack is full");
        ++_top;
        _data[_top] = item;
    }

    int pop()
    {
        if (size() == 0)
            exit_error("pop() from empty stack");

        --_top;
        return _data[_top + 1];
    }

    bool is_full()
    {
        return size() == _capacity;
    }
};

/*
MinHeap with max-capacity.
Aaain data array pair<int,int> is supplied used to avoid K_MAX size memory allocation in each test case
so time complexity per test case is still O(used size) and not O(K_MAX)

CRUX: Supports priority update for elements already in the heap
_index[VPN] stores index of VPN in the heap array 
*/
class MinHeap
{
public:
    pair<int, int> *_data;
    int _capacity;
    int _size;
    int *_index;
    bool *_in_cache;
    inline int parent(int i)
    {
        return (i - 1) / 2;
    }

    inline int left_child(int i)
    {
        return 2 * i + 1;
    }

    inline int right_child(int i)
    {
        return 2 * i + 2;
    }

    void _heapify_down(int idx)
    {

        while (true)
        {
            int left = left_child(idx);
            int right = right_child(idx);
            int least = idx;

            if (left < _size && _data[left].second < _data[least].second)
                least = left;
            if (right < _size && _data[right].second < _data[least].second)
                least = right;

            if (least != idx)
            {
                swap(_data[least], _data[idx]);
                swap(_index[_data[least].first], _index[_data[idx].first]);
                idx = least;
            }

            else
                return;
        }
    }

    void _heapify_up(int idx)
    {
        while (idx != 0)
        {
            int par = parent(idx);
            if (_data[idx].second < _data[par].second)
            {
                swap(_data[idx], _data[par]);
                swap(_index[_data[idx].first], _index[_data[par].first]);
                idx = par;
            }
            else
                return;
        }
    }

    void _add(int vpn, int priority)
    {
        if (_size == _capacity)
            exit_error("heap is full");

        ++_size;
        _data[_size - 1] = make_pair(vpn, priority);
        _index[vpn] = _size - 1;
        _in_cache[vpn] = true;
        _heapify_up(_size - 1);
    }

public:
    pair<int, int> _pop()
    {
        if (_size == 0)
            exit_error("pop() from empty heap");

        pair<int, int> root = _data[0];

        _in_cache[root.first] = false;

        _data[0] = _data[_size - 1];
        _index[_data[0].first] = 0;
        --_size;
        _heapify_down(0);
        return root;
    }

public:
    MinHeap(int max_elements, bool *inTLB, int *index_arr, pair<int, int> *heap_arr)
    {
        _capacity = max_elements;
        _size = 0;
        _data = heap_arr;

        _index = index_arr;
        _in_cache = inTLB;
    }

    bool update(int vpn, int priority)
    {

        if (!_in_cache[vpn]) // miss
        {
            if (_size == _capacity)
                _pop();

            _add(vpn, priority);
            return false;
        }
        /*
        this is the update operation
        look up the VPN entry in the heap in O(1) time then update the priority
        then heapify up or down to maintain the heap property
        */
        
        else  // hit
        {
            int heap_idx = _index[vpn];
            _data[heap_idx].second = priority;
            // exactly one of these two "acts" in a single call
            _heapify_down(heap_idx);
            _heapify_up(heap_idx);
            return true;
        }
    }

    bool empty()
    {
        return _size == 0;
    }
};

// set inTLB[VPN] to false for all VPN in queries ONLY
void cool_cache(int *queries, int N, bool *inTLB)
{
    for (int i = 0; i < N; i++)
        inTLB[queries[i]] = false;
}


/* 
index_arr[i] = index of next occurence of VPN queries[i] in queries. N if no other occurence
an entry future[i] is useful if and only if inTLB[queries[i]] is true
*/
void update_future_array(int *queries, int N, bool *inTLB, int *index_arr, int *future)
{
    for (int i = N - 1; i >= 0; i--)
    {
        int page = queries[i];
        if (inTLB[page])
            future[i] = index_arr[page];
        else
        {
            inTLB[page] = true;
            future[i] = N;
        }
        index_arr[page] = i;
    }
}
/*
CRUX: the new priority is -future[i] where future[i] is the next occurence of VPN queries[i] in queries
If futute[i] is large (VPN = queries[i] will be used very far in the future) then -future[i] is small
SO that VPN floats to the top of the MinHeap
*/
int run_OPT(int *queries, int N, int K, bool *inTLB, int *index_arr, int *future, pair<int, int> *heap_arr)
{
    cool_cache(queries, N, inTLB);
    update_future_array(queries, N, inTLB, index_arr, future);
    cool_cache(queries, N, inTLB);
    MinHeap cache(K, inTLB, index_arr, heap_arr);
    int hits = 0;
    for (int i = 0; i < N; i++)
    {

        int VPN = queries[i];
        hits += cache.update(VPN, -future[i]);
    
    }

    return hits;
}

/*
CRUX: the new priority is i where i is the next occurence of VPN queries[i] in queries i.e. i is the timestap of last VPN usage
if i is large (VPN = queries[i] was used very recently) so it sinks down the MinHeap
So that VPN is evicted from the MinHeap first
*/
int run_LRU(int *queries, int N, int K, bool *inTLB, int *index_arr, pair<int, int> *heap_arr)
{

    cool_cache(queries, N, inTLB);
    MinHeap cache(K, inTLB, index_arr, heap_arr);
    int hits = 0;
    for (int i = 0; i < N; i++)
    {
        int VPN = queries[i];
        hits += cache.update(VPN, i);
    }

    return hits;
}
int run_LIFO(int *queries, int N, int K, bool *inTLB, int *stack_arr)
{
    cool_cache(queries, N, inTLB);
    Stack cache(K, stack_arr); // to figure which VPN to evict
    int hits = 0;
    for (int i = 0; i < N; i++)
    {
        int VPN = queries[i];

        if (inTLB[VPN]) // hit
            ++hits;
        else if (!cache.is_full())
        {
            cache.push(VPN);
            inTLB[VPN] = true;
        }
        else
        {
            int evict = cache.pop();
            inTLB[evict] = false;
            inTLB[VPN] = true;
            cache.push(VPN);
        }
    }

    return hits;
}
/*
Same as LIFO but uses a queue instead of a stack
*/
int run_FIFO(int *queries, int N, int K, bool *inTLB, int *queue_arr)
{

    cool_cache(queries, N, inTLB);
    Queue cache(K, queue_arr); // to figure which VPN to evict
    int hits = 0;
    for (int i = 0; i < N; i++)
    {
        int VPN = queries[i];
        if (inTLB[VPN]) // hit
            ++hits;
        else if (!cache.is_full())
        {
            cache.push(VPN);
            inTLB[VPN] = true;
        }
        else
        {
            int evict = cache.pop();
            inTLB[evict] = false;
            inTLB[VPN] = true;
            cache.push(VPN);
        }
    }

    return hits;
}
inline int get_log2(int x)
{
    return __builtin_ctz(x);
}
void get_answer(int *queries, bool *inTLB, int *index_arr, int *future, int *queue_arr, pair<int, int> *heap_arr)
{
    int S, P, K, N;
    uint32_t address;
    scanf("%d %d %d %d", &S, &P, &K, &N); // much faster than cin
    int page_bits = get_log2(P) + LOG2_B_IN_KB;
    for (int i = 0; i < N; i++)
    {
        scanf("%x", &address);
        queries[i] = (address >> page_bits);
    }
    printf("%d ", run_FIFO(queries, N, K, inTLB, queue_arr));
    printf("%d ", run_LIFO(queries, N, K, inTLB, queue_arr));
    printf("%d ", run_LRU(queries, N, K, inTLB, index_arr, heap_arr));
    printf("%d\n", run_OPT(queries, N, K, inTLB, index_arr, future, heap_arr));

}

int main()
{

    /*
    All dynamic memory is allocated once and used in each test case. This saves time.
    */
    int *queries = new int[MAX_N];
    bool *inTLB = new bool[MAX_PAGES];
    int *queue_arr = new int[MAX_CACHE];
    int *index_arr = new int[MAX_PAGES];
    int *future = new int[MAX_PAGES];
    pair<int, int> *heap_arr = new pair<int, int>[MAX_CACHE];

    int t;
    scanf("%d", &t);
    for (int i = 0; i < t; i++)
    {
        get_answer(queries, inTLB, index_arr, future, queue_arr, heap_arr);
    }

    // delete[] queries;
    // delete[] inTLB;
    // delete[] queue_arr;
    // delete[] index_arr;
    // delete[] future;
    // delete[] heap_arr;
    return 0;
}