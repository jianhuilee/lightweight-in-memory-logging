#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>

/* Ref: https://preshing.com/20120522/lightweight-in-memory-logging/ */
struct event_log {
    pthread_t tid;
    /* Message string */
    const char *msg;
    /* Extra information */
    uint32_t param;
};

#define BUFFER_SIZE (65536)

struct event_log g_events[BUFFER_SIZE] = {0};
int32_t g_pos = -1;

extern struct event_log g_events[BUFFER_SIZE];
extern int32_t g_pos;

/* You have to use static declaration before inline in C99 in the same translation unit.
 * https://stackoverflow.com/questions/19068705/undefined-reference-when-calling-inline-function
 */
static inline void logging(const char *msg, uint32_t param)
{
    /* Get event index */
    uint32_t index =  __sync_fetch_and_add(&g_pos, 1);
    struct event_log *event = g_events + (index & (BUFFER_SIZE - 1));

    event->tid = pthread_self();
    event->msg = msg;
    event->param = param;
}

void print_all_log()
{
    int i;

    for (i = 0; i < BUFFER_SIZE; i++) {
        struct event_log *event = &g_events[i];
        if (event->msg)
            printf("0x%lX: %s\n", event->tid, event->msg);
    }
}

#define threads (1024)
#define MSG_SIZE (1024)
void *thread_func(void *parm)
{
    logging("Test", 0);
    pthread_exit((void *)0);
}

int main(int argc, const char *argv[])
{
    pthread_t threadid[threads];
    int thread_stat[threads];
    int i, status;

    for (i = 0; i < threads; i++) {
        status = pthread_create(&threadid[i],
                                NULL,
                                thread_func,
                                NULL);

        if (status <  0) {
            printf("pthread_create %d failed, errno=%d", i, errno);
            return 2;
        }
    }

    for (i = 0; i < threads; i++) {
        status = pthread_join(threadid[i], (void *)&thread_stat[i]);

        if (status <  0) {
            printf("pthread_join failed, thread %d, errno=%d\n", i, errno);
        }

        if (thread_stat[i] != 0)   {
            printf("bad thread status, thread %d, status=%d\n", i,
                   thread_stat[i]);
        }
    }

    print_all_log();
    return 0;
}
