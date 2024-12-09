#pragma once
#include "helper.h"




void FCFS(Process processes[], int n)
{
    char *filename = "result_offline_FCFS.csv";
    if (!write_header(filename))
        unhandled_exit("fopen failed!");

    uint64_t scheduler_start_time = get_current_time_millis();

    for (int i = 0; i < n; i++)
    {
        processes[i].started = processes[i].finished = false;
        processes[i].arrival_time = get_current_time_millis();
    }

    for (int i = 0; i < n; i++)
    {
        Process *process = &processes[i];
        init_process_run_for_quantum(process, 0, true, scheduler_start_time);
        describe_process(process, filename);
    }
}
void RoundRobin(Process processes[], int n, uint64_t quantum)
{

    char *filename = "result_offline_RR.csv";
    if (!write_header(filename))
        unhandled_exit("fopen failed!");

    uint64_t scheduler_start_time = get_current_time_millis();

    Queue *queue = create_queue(n);
    for (int i = 0; i < n; i++)
    {
        processes[i].started = processes[i].finished = false;
        processes[i].arrival_time = get_current_time_millis();
        push(queue, &processes[i]);
    }
    while (!is_empty(queue))
    {
        Process *process = pop(queue);
        if (!process->started)
            init_process_run_for_quantum(process, quantum, false, scheduler_start_time);
        else
            run_process_for_quantum(process, quantum, scheduler_start_time);

        if (!process->finished)
            push(queue, process);
        else
            describe_process(process, filename);
    }
    free_queue(queue);
}

void MultiLevelFeedbackQueue(Process processes[], int n, uint64_t quantum0, uint64_t quantum1, uint64_t quantum2, uint64_t boostTime)
{

    char *filename = "result_offline_MLFQ.csv";
    if (!write_header(filename))
        unhandled_exit("fopen failed!");

    uint64_t scheduler_start_time = get_current_time_millis();

    int NUM_QUEUES = 3;
    uint64_t quantums[] = {quantum0, quantum1, quantum2};
    Queue *queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++)
        queues[i] = create_queue(n);

    for (int i = 0; i < n; i++)
    {
        processes[i].started = processes[i].finished = false;
        processes[i].arrival_time = get_current_time_millis();
        push(queues[0], &processes[i]);
    }

    uint64_t last_boost_time = get_current_time_millis();
    while (!all_empty(queues, NUM_QUEUES))
    {
        for (int priority = 0; priority < NUM_QUEUES; priority++)
            if (!is_empty(queues[priority]))
            {
                Process *process = pop(queues[priority]);
                if (!process->started)
                    init_process_run_for_quantum(process, quantums[priority], false, scheduler_start_time);

                else
                    run_process_for_quantum(process, quantums[priority], scheduler_start_time);
                if (!process->finished)
                {
                    int lower_queue_id = (priority == NUM_QUEUES - 1) ? priority : priority + 1;
                    push(queues[lower_queue_id], process);
                }
                else
                    describe_process(process, filename);

                uint64_t current_time = get_current_time_millis();
                if (current_time - last_boost_time >= boostTime)
                {
                    boost_priorities(queues, NUM_QUEUES);
                    last_boost_time = current_time;
                }

                break;
            }
    }

    for (int i = 0; i < NUM_QUEUES; i++)
        free_queue(queues[i]);
}
