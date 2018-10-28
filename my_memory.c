//
// Created by bh398 on 10/21/18.
//

#include "my_memory_t.h"
#include "my_pthread_t.h"
#include <malloc.h>
#include <math.h>

//#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define THREADREQ 1
#define PHYSICAL_MEMORY_SIZE (8*1024*1024)
#define VIRTUAL_MEMORY_SIZE (3*8*1024*1024)
#define PAGE_SIZE (4*1024)
#define MAX_PAGE_NUMBER VIRTUAL_MEMORY_SIZE/PAGE_SIZE
#define MAX_FRAME_NUMBER PHYSICAL_MEMORY_SIZE/PAGE_SIZE
#define MAX_PAGE_A_THREAD_CAN_USE 20
#define KERNEL_SPACE 100

typedef struct _page_table_entry {
    int page_no;
    my_pthread_t page_owner;
    int evicted;
} page_table_entry;

struct {
    unsigned allocated:1;
    unsigned size:31;
} Header;

typedef struct __node_t {
    long int head;
} node_t;

static int initialized = 0;

char *memory;

page_table_entry page_table[MAX_PAGE_NUMBER];

void initialize_a_page(int page_no) {
    node_t *p = &memory[page_no * PAGE_SIZE];
    p->head =
}

void initialize() {
    printf("Memory environment Initializing! \n");
    memory = (char *) malloc(sizeof(char) * PHYSICAL_MEMORY_SIZE);
    for (int i = 0; i < MAX_PAGE_NUMBER; i++) {
        page_table[i].page_no = i;
        page_table[i].page_owner = -1;
        if (i < MAX_FRAME_NUMBER) page_table[i].evicted = -1;
        else if (i < 2 * MAX_FRAME_NUMBER) page_table[i].evicted = 0;
        else page_table[i].evicted = 1;
    }
    initialized = 1;
    printf("Memory environment complete! \n");
}

int find_a_free_page(memory_control_block *memo_block) {
    int i = KERNEL_SPACE;
    for (; i < MAX_PAGE_NUMBER && page_table[i].page_owner != -1; i++);
    if (i == MAX_PAGE_NUMBER) {
        printf("No free page for current page now! \n");
        return -1;
    }
    int free_page = i;
    for (i = 0; i < MAX_PAGE_A_THREAD_CAN_USE && memo_block->page[i] == -1; i++);
    if (i == MAX_PAGE_A_THREAD_CAN_USE) {
        printf("Current thread is using too many pages! \n");
        return -1;
    }
    memo_block->page[i] = free_page;
    memo_block->current_page = free_page;
}

int swap_file(int page_to_swapin) {
    return 0;
}

char *myallocate(int x, char *file, int line, int thread_req) {
    if (initialized == 0) {
        initialize();
    }
    if (x > PAGE_SIZE) {
        printf("This variable is larger than a whole page size! \n");
        return NULL;
    }
    thread_control_block *current_thread = get_current_running_thread();
    if (current_thread->memo_block == NULL) {
        current_thread->memo_block = (memory_control_block *) malloc(sizeof(memory_control_block));
        current_thread->memo_block->current_page = -1;
        current_thread->memo_block->next_free - 1;
        for (int i = 0; i < MAX_PAGE_A_THREAD_CAN_USE; i++) {
            current_thread->memo_block->page = -1;
        }
    }
    memory_control_block *memo_block = current_thread->memo_block;
    if (PAGE_SIZE - memo_block->next_free <= x) {
        if (find_a_free_page(memo_block) == -1) {
            printf("Current thread needs a new page, but there is no free one !\n");
            return NULL;
        }
    }
    int current_page = memo_block->current_page;
    if (page_table[current_page].evicted) {
        swap_file(current_page);
    }
    char *pointer = &memory[current_page * PAGE_SIZE + memo_block->next_free];
    return pointer;
}


