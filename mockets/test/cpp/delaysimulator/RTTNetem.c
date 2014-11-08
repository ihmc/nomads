#include "RTTNetem.h"

int main (int argc, char *argv[])
{
    flog = fopen ("netemlog.txt", "wb");
    netemSlowRamp();
    fclose (flog);
    return 0;
}

int64 getTimeInMilliseconds (void)
{
    struct timeval tv;
    if (gettimeofday (&tv,0) == -1)
        return 0;
    else
        return (int64)tv.tv_sec* (int64) 1000 + (int64) tv.tv_usec / (int64) 1000;
}

void writeLog (char *pLogMsg, int64 i64Timestamp, uint32 ui32Delay)
{
    if (pLogMsg != NULL) {
        fwrite (pLogMsg, sizeof (char), strlen (pLogMsg), flog);
    }
    else {
        char pPrint[256];
        sprintf (pPrint, "%llu\t%f\n", i64Timestamp, ((float)ui32Delay / (float)1000));
        fwrite (pPrint, sizeof (char), strlen (pPrint), flog);
    }
}

void netemStep (void)
{
    int64 i64TStart;
    uint32 ui32Delay = 20000;

    channel_state_info_t state_info;

    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         FOURTY_PERCENT; /* 214748365random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */

    init_netem();
    i64TStart = getTimeInMilliseconds();
    setup_netem (&state_info);
    writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    
    while (getTimeInMilliseconds() < (i64TStart + 30000)) {
            usleep (PRINT_DELAY);
            writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    }
    printf ("Changing delay\n");
    ui32Delay *= 2;
    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         FOURTY_PERCENT; /* random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
    setup_netem (&state_info);
    
    writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    
    while (getTimeInMilliseconds() < (i64TStart + 60000)) {
            usleep (PRINT_DELAY);
            writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    }
    close_netem();
}

void netemRamp (void)
{
    int64 i64TStart, i64TLastModify;
    uint32 ui32Delay = 20000;

    channel_state_info_t state_info;

    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         214748365; /* random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */

    init_netem();
    i64TStart = getTimeInMilliseconds();
    setup_netem (&state_info);
    writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    
    while (getTimeInMilliseconds() < (i64TStart + 15000)) {
            usleep (PRINT_DELAY);
            writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    }
    
    printf ("Changing delay\n");
    ui32Delay += 50;
    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         214748365; /* random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
    i64TLastModify = getTimeInMilliseconds();
    setup_netem (&state_info);
    
    while ((getTimeInMilliseconds() < (i64TStart + 60000)) || (ui32Delay < 40000)) {
            usleep (PRINT_DELAY);
            writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
        if ((getTimeInMilliseconds() >= (i64TLastModify + RAMP_INCREASE_DELAY)) && (ui32Delay < 40000)) {
            ui32Delay += 50;
            memset (&state_info, 0, sizeof(state_info));
            state_info.delay          = ui32Delay; /* added delay (us) */
            state_info.jitter         =         0; /* random jitter in latency (us) */
            state_info.loss_rate      =         214748365; /* random packet loss (0=none ~0=100%) */
            state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
            state_info.queue_size     =      1000; /* fifo limit (packets) */
            state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
            i64TLastModify = getTimeInMilliseconds();
            setup_netem (&state_info);
        }
    }
    close_netem();
}

void netemSlowRamp (void)
{
    int64 i64TStart, i64TLastModify;
    uint32 ui32Delay = 20000;
    int updown = 1;

    channel_state_info_t state_info;

    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         214748365; /* random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */

    init_netem();
    i64TStart = getTimeInMilliseconds();
    setup_netem (&state_info);
    writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    
    while (getTimeInMilliseconds() < (i64TStart + 5000)) {
            usleep (PRINT_DELAY);
            writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    }
    
    printf ("Changing delay\n");
    ui32Delay += 5000;
    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         FOURTY_PERCENT; /* random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
    i64TLastModify = getTimeInMilliseconds();
    setup_netem (&state_info);
    
    while ((getTimeInMilliseconds() < (i64TStart + 100000))) {
            usleep (PRINT_DELAY);
            writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
        if ((getTimeInMilliseconds() >= (i64TLastModify + 10000)) && (ui32Delay > 20000) && (ui32Delay <=40000)) {
            if (ui32Delay == 40000) updown = -1;
            ui32Delay += updown * 5000;
            memset (&state_info, 0, sizeof(state_info));
            state_info.delay          = ui32Delay; /* added delay (us) */
            state_info.jitter         =         0; /* random jitter in latency (us) */
            state_info.loss_rate      =         FOURTY_PERCENT; /* random packet loss (0=none ~0=100%) */
            state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
            state_info.queue_size     =      1000; /* fifo limit (packets) */
            state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
            i64TLastModify = getTimeInMilliseconds();
            setup_netem (&state_info);
        }
    }
    close_netem();
}

void netemSteps (void)
{
    int64 i64TStart, i64TLastModify, i64TNow;
    int i=0;
    uint32 ui32Delay = 20000;

    channel_state_info_t state_info;

    memset (&state_info, 0, sizeof(state_info));
    state_info.delay          = ui32Delay; /* added delay (us) */
    state_info.jitter         =         0; /* random jitter in latency (us) */
    state_info.loss_rate      =         214748365; /* random packet loss (0=none ~0=100%) */
    state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
    state_info.queue_size     =      1000; /* fifo limit (packets) */
    state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
    init_netem();
    i64TStart = i64TLastModify = getTimeInMilliseconds();
    writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
    setup_netem (&state_info);
    while ((i64TNow = getTimeInMilliseconds()) <= (i64TStart + 60000)) {
        usleep (PRINT_DELAY);
        writeLog (NULL, getTimeInMilliseconds(), ui32Delay);
        if (i64TNow >= i64TLastModify + 5000) {
            if (i == 0) {
                i = 1;
                ui32Delay = 40000;
            }
            else {
                i = 0;
                ui32Delay = 20000;
            }
            memset (&state_info, 0, sizeof(state_info));
            state_info.delay          = ui32Delay; /* added delay (us) */
            state_info.jitter         =         0; /* random jitter in latency (us) */
            state_info.loss_rate      =         214748365; /* random packet loss (0=none ~0=100%) */
            state_info.duplicate_rate =         0; /* random packet dup  (0=none ~0=100%) */
            state_info.queue_size     =      1000; /* fifo limit (packets) */
            state_info.reorder_gap    =         0; /* re-ordering gap (0 for none) */
            i64TLastModify = getTimeInMilliseconds();
            setup_netem (&state_info);
        }
    }
    close_netem();
}

