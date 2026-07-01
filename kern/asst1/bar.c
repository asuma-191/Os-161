#include <types.h>
#include <lib.h>
#include <synch.h>
#include "bar.h"

static struct bar_order *order_queue[BAR_QUEUE_SIZE];
static unsigned queue_head;
static unsigned queue_tail;

static struct semaphore *queue_empty;
static struct semaphore *queue_full;
static struct semaphore *queue_mutex;

static struct lock *bottle_locks[BAR_BOTTLES];
static unsigned bottle_counts[BAR_BOTTLES];

static
void
check_bottle(int bottle)
{
        KASSERT(bottle >= BOTTLE_1);
        KASSERT(bottle <= BOTTLE_10);
}

void
bar_open(void)
{
        unsigned i;

        queue_head = 0;
        queue_tail = 0;
        for (i = 0; i < BAR_QUEUE_SIZE; i++) {
                order_queue[i] = NULL;
        }

        queue_empty = sem_create("bar queue empty", BAR_QUEUE_SIZE);
        queue_full = sem_create("bar queue full", 0);
        queue_mutex = sem_create("bar queue mutex", 1);
        if (queue_empty == NULL || queue_full == NULL || queue_mutex == NULL) {
                panic("bar_open: could not create queue semaphores\n");
        }

        for (i = 0; i < BAR_BOTTLES; i++) {
                bottle_counts[i] = 0;
                bottle_locks[i] = lock_create("bar bottle");
                if (bottle_locks[i] == NULL) {
                        panic("bar_open: could not create bottle lock\n");
                }
        }
}

void
bar_close(void)
{
        unsigned i;

        for (i = 0; i < BAR_BOTTLES; i++) {
                lock_destroy(bottle_locks[i]);
                bottle_locks[i] = NULL;
        }

        sem_destroy(queue_empty);
        sem_destroy(queue_full);
        sem_destroy(queue_mutex);
        queue_empty = NULL;
        queue_full = NULL;
        queue_mutex = NULL;
}

void
place_order(struct bar_order *order)
{
        P(queue_empty);
        P(queue_mutex);

        order_queue[queue_tail] = order;
        queue_tail = (queue_tail + 1) % BAR_QUEUE_SIZE;

        V(queue_mutex);
        V(queue_full);
}

struct bar_order *
take_order(void)
{
        struct bar_order *order;

        P(queue_full);
        P(queue_mutex);

        order = order_queue[queue_head];
        order_queue[queue_head] = NULL;
        queue_head = (queue_head + 1) % BAR_QUEUE_SIZE;

        V(queue_mutex);
        V(queue_empty);

        return order;
}

static
void
sort_bottles(int bottles[], unsigned count)
{
        unsigned i, j;

        for (i = 0; i < count; i++) {
                for (j = i + 1; j < count; j++) {
                        if (bottles[j] < bottles[i]) {
                                int tmp = bottles[i];
                                bottles[i] = bottles[j];
                                bottles[j] = tmp;
                        }
                }
        }
}

static
unsigned
unique_requested_bottles(const struct bar_order *order, int bottles[])
{
        unsigned count, i, j;

        count = 0;
        for (i = 0; i < BAR_INGREDIENTS; i++) {
                int bottle = order->requested[i];
                bool seen = false;

                if (bottle == NO_INGREDIENT) {
                        continue;
                }
                check_bottle(bottle);

                for (j = 0; j < count; j++) {
                        if (bottles[j] == bottle) {
                                seen = true;
                                break;
                        }
                }
                if (!seen) {
                        bottles[count++] = bottle;
                }
        }

        sort_bottles(bottles, count);
        return count;
}

void
mix(struct bar_order *order)
{
        int bottles[BAR_INGREDIENTS];
        unsigned count, i;

        count = unique_requested_bottles(order, bottles);

        for (i = 0; i < count; i++) {
                lock_acquire(bottle_locks[bottles[i] - 1]);
        }

        for (i = 0; i < BAR_INGREDIENTS; i++) {
                int bottle = order->requested[i];
                order->glass.contents[i] = bottle;
                if (bottle != NO_INGREDIENT) {
                        bottle_counts[bottle - 1]++;
                }
        }

        for (i = count; i > 0; i--) {
                lock_release(bottle_locks[bottles[i - 1] - 1]);
        }
}

void
finish_order(struct bar_order *order)
{
        V(order->served);
}

void
stop_bartender(void)
{
        place_order(NULL);
}

unsigned
bottle_usage(int bottle)
{
        check_bottle(bottle);
        return bottle_counts[bottle - 1];
}
