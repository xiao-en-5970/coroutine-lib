#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <stdint.h>
#include <time.h>
//stack max size
#define STACK_SIZE 8*8192

//coroutine status
enum co_status {
    CO_NEW = 1,
    CO_RUNNING,
    CO_WAITING,
    CO_DEAD,
};
// coroutine struct
struct co {
    char name[128];
    void (*func)(void *);
    void *arg;
    enum co_status status;
    struct co *waiter;
    ucontext_t context;
    uint8_t stack[STACK_SIZE];
    struct co *next; // 链接到下一个协程
};


// void co_entry();

//main_coroutine
struct co main_co;
//init the current coroutine with main
struct co *current = &main_co;
// init the listnode
struct co *co_list = NULL; // 所有协程的链表

// listnode_delete
void del(struct co*c)
{
    struct co*p = co_list;
    if(strcmp(c->name,p->name))
    {
        co_list=co_list->next;
        free(c);
        return;
    }
    for(;p!=NULL;p=p->next)
    {
        if(strcmp(c->name,p->next->name))
        {
            p->next = p->next->next;
            free(c);
            return;
        }
    }
    return;
}
//yield
void co_yield() {
    struct co *prev = current;

    // 将当前协程状态设置为等待中（除非它已经结束）
    if (prev->status != CO_DEAD) {
        prev->status = CO_WAITING;
    }

    // 从现有协程列表中随机选择一个未结束的协程
    int num_cos = 0;
    struct co *co_iter = co_list;
    while (co_iter) {
        if (co_iter->status != CO_DEAD) {
            num_cos++;
        }
        co_iter = co_iter->next;
    }

    if (num_cos > 0) {
        int random_index = rand() % num_cos;
        co_iter = co_list;
        while (co_iter) {
            if (co_iter->status != CO_DEAD) {
                if (random_index == 0) {
                    current = co_iter;
                    current->status = CO_RUNNING;
                    break;
                }
                random_index--;
            }
            co_iter = co_iter->next;
        }
    } else {
        current = &main_co;
    }

    swapcontext(&prev->context, &current->context);
}
// run!
void co_entry() {
    if (current && current->func) {
        current->func(current->arg);
    }
    current->status = CO_DEAD;
    // if (current->waiter) {
    //     current->waiter->status = CO_RUNNING;
    // }
    co_yield();
}
// coroutine create
struct co* co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *co = (struct co *)malloc(sizeof(struct co));
    if (!co) {
        perror("malloc");
        return NULL;
    }
    // co_list[co_count++] = co;
    strncpy(co->name, name, sizeof(co->name) - 1);
    co->name[sizeof(co->name) - 1] = '\0';
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    // co->waiter = NULL;
    co->next = NULL;
    getcontext(&co->context);

    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = STACK_SIZE;
    co->context.uc_link = &main_co.context;
    makecontext(&co->context, co_entry, 0);
        // 将新协程添加到协程列表中
    co->next = co_list;
    co_list = co;
    return co;
}



void co_wait(struct co *co) {
    
     while (co->status != CO_DEAD) {
        // co->waiter = current;
        // current->status = CO_WAITING;
        // current = co;
        // co->status = CO_RUNNING;
        // swapcontext(&current->waiter->context, &co->context);
        co_yield();
    }
    del(co);
}


