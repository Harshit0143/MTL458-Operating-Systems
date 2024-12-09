#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "online_schedulers.h"

int main()
{
    // ShortestJobFirst();
    
    // Uncomment below to run MultiLevelFeedbackQueue
    MultiLevelFeedbackQueue(100, 200, 400, 1000);

    return 0;
}
