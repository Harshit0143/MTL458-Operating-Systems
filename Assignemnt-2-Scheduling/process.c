#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/*
./process <load in secinds> <process name>
<process name> does not have have functionaliy, just to identify process while running
*/
double get_cpu_time_in_seconds()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
        return 1;
    

    double burst_time = atof(argv[1]);
    double cpu_time_consumed = 0.0;

    while (cpu_time_consumed < burst_time)
        cpu_time_consumed = get_cpu_time_in_seconds();
    return 0;
}
