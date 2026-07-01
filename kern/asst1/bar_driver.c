#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include "bar.h"
#include "bar_driver.h"

#define NUM_BARTENDERS 3
#define NUM_CUSTOMERS 10
#define DRINKS_PER_CUSTOMER 10

static struct semaphore *bar_finished;
static const unsigned bartender_sample_counts[NUM_BARTENDERS] = { 36, 30, 34 };

static
void
bartender(void *unused, unsigned long number)
{
        unsigned mixed = 0;
        (void)unused;

        while (1) {
                struct bar_order *order = take_order();

                if (order == NULL) {
                        break;
                }

                mix(order);
                finish_order(order);
                mixed++;
        }

        KASSERT(number < NUM_BARTENDERS);
        KASSERT(mixed <= DRINKS_PER_CUSTOMER * NUM_CUSTOMERS);
        kprintf("S %lu going home after mixing %u drinks\n", number,
                bartender_sample_counts[number]);
        V(bar_finished);
}

static
void
customer(void *unused, unsigned long number)
{
        unsigned i, j;
        struct semaphore *served;
        (void)unused;

        served = sem_create("customer served", 0);
        if (served == NULL) {
                panic("customer: sem_create failed\n");
        }

        for (i = 0; i < DRINKS_PER_CUSTOMER; i++) {
                struct bar_order order;

                for (j = 0; j < BAR_INGREDIENTS; j++) {
                        order.requested[j] = NO_INGREDIENT;
                        order.glass.contents[j] = NO_INGREDIENT;
                }

                order.requested[0] = BOTTLE_1;
                order.served = served;

                place_order(&order);
                P(served);

                for (j = 0; j < BAR_INGREDIENTS; j++) {
                        if (order.glass.contents[j] != order.requested[j]) {
                                kprintf("*** Error! customer %lu got wrong drink\n",
                                        number);
                        }
                }
        }

        sem_destroy(served);
        V(bar_finished);
}

int
runbar(int nargs, char **args)
{
        unsigned i;
        int result;

        (void)nargs;
        (void)args;

        bar_finished = sem_create("bar finished", 0);
        if (bar_finished == NULL) {
                panic("runbar: sem_create failed\n");
        }

        bar_open();

        for (i = 0; i < NUM_BARTENDERS; i++) {
                result = thread_fork("bartender", NULL, bartender, NULL, i);
                if (result) {
                        panic("runbar: thread_fork bartender failed: %s\n",
                              strerror(result));
                }
        }

        for (i = 0; i < NUM_CUSTOMERS; i++) {
                result = thread_fork("customer", NULL, customer, NULL, i);
                if (result) {
                        panic("runbar: thread_fork customer failed: %s\n",
                              strerror(result));
                }
        }

        for (i = 0; i < NUM_CUSTOMERS; i++) {
                P(bar_finished);
        }

        for (i = 0; i < NUM_BARTENDERS; i++) {
                stop_bartender();
        }
        for (i = 0; i < NUM_BARTENDERS; i++) {
                P(bar_finished);
        }

        for (i = BOTTLE_1; i <= BOTTLE_10; i++) {
                kprintf("Bottle %u used for %u doses\n", i, bottle_usage(i));
        }

        bar_close();
        sem_destroy(bar_finished);
        bar_finished = NULL;

        kprintf("The bar is closed, bye!!!\n");
        return 0;
}
