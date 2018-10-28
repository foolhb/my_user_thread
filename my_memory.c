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
#define NUMBER_OF_PAGES VIRTUAL_MEMORY_SIZE/PAGE_SIZE
#define NUMBER_OF_FRAMES PHYSICAL_MEMORY_SIZE/PAGE_SIZE
#define MAX_PAGE_A_THREAD_CAN_USE 20
#define NUMBER_OF_KERNEL_PAGES 100
#define KERNEL_MODE 1
#define USER_MODE 0

typedef struct _page_table_entry {
    int page_no;
    my_pthread_t page_owner;
    int position;
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

page_table_entry page_table[NUMBER_OF_PAGES];

void page_initialize(int page_no) {
    node_t *p = &memory[page_no * PAGE_SIZE];
    p->head = 4095 << 1;
}

void initialize() {
    initialized = 1;
    printf("Memory environment Initializing! \n");
    memory = (char *) malloc(sizeof(char) * PHYSICAL_MEMORY_SIZE);
    for (int i = 0; i < NUMBER_OF_PAGES; i++) {
        page_table[i].page_no = i;
        page_table[i].page_owner = -1;
        if (i < NUMBER_OF_FRAMES) page_table[i].position = -1;
        else if (i < 2 * NUMBER_OF_FRAMES) page_table[i].position = 0;
        else page_table[i].position = 1;
        page_initialize(i);
    }
    printf("Memory environment complete! \n");
}

int find_a_free_page(int kernel_mode) {
    int i = kernel_mode ? 0 : 0 + NUMBER_OF_KERNEL_PAGES;
    for (; i < NUMBER_OF_PAGES && page_table[i].page_owner != -1; i++);
    if (i == NUMBER_OF_PAGES) {
        printf("All pages are used!\n");
        return -1;
    } else return i;
}

int swap_file(int page_to_swapin) {
    return 0;
}

void *allocate_on_page(int size, int page_no) {
    char *start = &memory[page_no * PAGE_SIZE];
    node_t *nodePtr = start;
    char *charPtr = start;
    int head = nodePtr->head, length = head >> 1, used = head & 1;
    while (charPtr - start < PAGE_SIZE - 2 && (head & 1 == 1 || length < size)) {
        charPtr += length + sizeof(node_t);
        nodePtr = charPtr;
        head = nodePtr->head;
        length = head >> 1;
        used = head & 1;
    }
    if (charPtr - start < PAGE_SIZE) {
        if (length > size + sizeof(node_t) + 1) {
            node_t *next = charPtr + size + sizeof(node_t);
            next->head = (length - size - sizeof(node_t)) << 1;
        }
        nodePtr->head += 1;
        return (void *) (charPtr + sizeof(node_t)); // might have problems?
    } else return NULL;
}

void mydeallocate(void *p) {
    node_t *nodePtr = p;
    (nodePtr->head)--;
    return;
}

void *myallocate(int x, char *file, int line, int thread_req) {
    if (initialized == 0) {
        initialize();
    }
    if (thread_req == USER_MODE) {
        if (x > PAGE_SIZE) {
            printf("Too large variable! \n");
            return NULL;
        }
        thread_control_block *current_thread = get_current_running_thread();
        if (current_thread->memo_block == NULL) {
            current_thread->memo_block = (memory_control_block *) malloc(sizeof(memory_control_block));
            current_thread->memo_block->current_page = -1;
            //current_thread->memo_block->next_free - 1;
            for (int i = 0; i < MAX_PAGE_A_THREAD_CAN_USE; i++) {
                current_thread->memo_block->page[i] = -1;
            }
        }
        memory_control_block *memo_block = current_thread->memo_block;
        void *pointer = NULL;
        //int current_page = memo_block->current_page;
        int i = 0;
        for (; memo_block->page[i] != -1; i++) {
            pointer = allocate_on_page(x, memo_block->page[i]);
            if (pointer != 0) break;
        }
        if (pointer == 0 && i < MAX_PAGE_A_THREAD_CAN_USE && memo_block->page[i] == -1) {
            memo_block->page[i] = find_a_free_page(USER_MODE);
            if (memo_block->page[i] == -1) return NULL;
            pointer = allocate_on_page(x, memo_block->page[i]);
        } else {
            printf("This thread is using too many pages! \n");
            return NULL;
        }
        if (page_table[memo_block->page[i]].position) {
            swap_file(memo_block->page[i]);
        }
        return pointer;
    } else if (thread_req = KERNEL_MODE) {
        if (x > PAGE_SIZE) {
            printf("Warning: kernel variable not allocated! \n");
            return NULL;
        }
        void *pointer = 0;
        for (int i = 0; i < NUMBER_OF_KERNEL_PAGES; i++) {
            pointer = allocate_on_page(x, i);
            if (pointer != 0) return pointer;
        }
        printf("Warning: kernel space full! \n");
        return NULL;
    }

}


