#include "helper.h"

void ShortestJobFirst()
{
    char *filename = "result_online_SJF.csv";
    if (!write_header(filename))
        unhandled_exit("fopen failed!");

    uint64_t scheduler_start_time = get_current_time_millis();

    set_nonblocking_io();
    Process *processes[MAX_PROCESSES] = {NULL};

    char input[MAX_COMMAND_LENGTH];
    size_t process_cnt = 0;
    while (true)
    {

        while (fgets(input, sizeof(input), stdin) != NULL)
        {

            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) == 0)
                continue;
            char *command_hash = generate_command_hash(input);

            if (strlen(command_hash) == 0)
            {
                free(command_hash);
                continue;
            }
            bool found = false;
            uint64_t expected_burst_time = get_average_burst_time(processes, process_cnt, command_hash, &found, DEFAULT_BURST_TIME);
            Process *process = create_process(input, command_hash, expected_burst_time);
            processes[process_cnt++] = process;
        }

        size_t shortest_job_index = get_SJF_index(processes, process_cnt);
        if (shortest_job_index == process_cnt)
            continue;
        Process *process = processes[shortest_job_index];
        init_process_run_for_quantum(process, 0, true, scheduler_start_time);
        describe_process(process, filename);

        if (!process->error)
        {
            bool found = false;
            uint64_t new_expected_burst_time = get_average_burst_time(processes, process_cnt, process->command_hash, &found, DEFAULT_BURST_TIME);
            update_expected_burst_time(processes, process_cnt, process->command_hash, new_expected_burst_time);
        }
    }
}

void MultiLevelFeedbackQueue(uint64_t quantum0, uint64_t quantum1, uint64_t quantum2, uint64_t boostTime)
{
    char *filename = "result_online_MLFQ.csv";
    if (!write_header(filename))
        unhandled_exit("fopen failed!");

    uint64_t scheduler_start_time = get_current_time_millis();

    set_nonblocking_io();
    Process *processes[MAX_PROCESSES] = {NULL};

    char input[MAX_COMMAND_LENGTH];
    size_t process_cnt = 0;

    int NUM_QUEUES = 3;
    uint64_t quantums[] = {quantum0, quantum1, quantum2};
    Queue *queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++)
        queues[i] = create_queue(MAX_PROCESSES);

    uint64_t last_boost_time = get_current_time_millis();
    while (true)
    {

        while (fgets(input, sizeof(input), stdin) != NULL)
        {

            input[strcspn(input, "\n")] = '\0';
            if (strlen(input) == 0)
                continue;
            char *command_hash = generate_command_hash(input);
            if (strlen(command_hash) == 0)
            {
                free(command_hash);
                continue;
            }
            bool found = false;
            uint64_t expected_burst_time = get_average_burst_time(processes, process_cnt, command_hash, &found, DEFAULT_BURST_TIME);
            Process *process = create_process(input, command_hash, expected_burst_time);
            processes[process_cnt++] = process;
            if (!found)
                push(queues[1], process);
            else if (process->expected_burst_time <= quantums[0])
                push(queues[0], process);
            else if (process->expected_burst_time <= quantums[1])
                push(queues[1], process);
            else
                push(queues[2], process);
        }

        if (all_empty(queues, NUM_QUEUES))
            continue;

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
}
