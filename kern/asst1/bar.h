#ifndef BAR_H
#define BAR_H

#define BAR_BOTTLES 10
#define BAR_INGREDIENTS 3
#define BAR_QUEUE_SIZE 16

#define NO_INGREDIENT 0
#define BOTTLE_1 1
#define BOTTLE_2 2
#define BOTTLE_3 3
#define BOTTLE_4 4
#define BOTTLE_5 5
#define BOTTLE_6 6
#define BOTTLE_7 7
#define BOTTLE_8 8
#define BOTTLE_9 9
#define BOTTLE_10 10

struct bar_glass {
        int contents[BAR_INGREDIENTS];
};

struct bar_order {
        int requested[BAR_INGREDIENTS];
        struct bar_glass glass;
        struct semaphore *served;
};

void bar_open(void);
void bar_close(void);
void place_order(struct bar_order *order);
struct bar_order *take_order(void);
void mix(struct bar_order *order);
void finish_order(struct bar_order *order);
void stop_bartender(void);
unsigned bottle_usage(int bottle);

#endif
