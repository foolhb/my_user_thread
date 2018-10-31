//
// Created by bh398 on 10/21/18.
//

#include "my_memory_t.h"
#include "my_pthread_t.h"
//#include "test_stub.h"
#include <malloc.h>
#include <math.h>
#include <string.h>

//#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define THREADREQ 1
#define PHYSICAL_MEMORY_SIZE (8*1024*1024)
#define VIRTUAL_MEMORY_SIZE (3*8*1024*1024)
#define PAGE_SIZE (4*1024)
#define NUMBER_OF_PAGES VIRTUAL_MEMORY_SIZE/PAGE_SIZE
#define NUMBER_OF_FRAMES PHYSICAL_MEMORY_SIZE/PAGE_SIZE
#define MAX_PAGE_A_THREAD_CAN_USE 20
#define NUMBER_OF_KERNEL_PAGES 100
#define NUMBER_OF_SHARED_PAGES 4
#define KERNEL_MODE 1
#define USER_MODE 0
#define FILENAME "virtual_memory_file"


//Pay attention to this: when searching free page,do not allocate kernel page to a thread!
//Modify anywhere you write like this!

/**
 * Page table entry to record page number, owner thread and position(position also indicates whether it is evicted)
 * Size: 12 Bytes
 */
typedef struct _page_table_entry {
    int page_no;
    my_pthread_t page_owner;
    int position;
} page_table_entry;

/**
 * A simple integer number to record the memory usage inside a page.
 * Structure: length of next memory chunk | whether the chunk is used
 *            15 bits                       1 bit
 * Size: 4 Bytes
 */
typedef struct __node_t {
    long int head;
} node_t;

static int initialized = 0;

char *memory;

page_table_entry page_table[NUMBER_OF_PAGES];

/**
 * Get the starting address of a page
 * @param page_no
 * @return a pointer that points to the start address
 */
char *get_page_address(int page_no) {
    //获取page的首地址，首先计算出frame number
    int frame_no = page_table[page_no].position;
    if (frame_no > NUMBER_OF_FRAMES) {
        printf("Page to access not in physical memory now\n");
        return NULL;
    }
    char *p = &memory[frame_no * PAGE_SIZE];
    return p;
}

/**
 * Initialize a new page, Give it a head to indicate that the whole page is empty
 * @param page_no page number
 */
void page_initialize(int page_no) {
    //根据page number初始化page
    //首先根据page number找到这个page所在的位置，如果不在memory中在文件中，报错
    //初始化记录内存使用记录，length = 4096 - 1，used = 0
//    int frame_no = page_table[page_no].position;
//    if (frame_no > NUMBER_OF_FRAMES) {
//        printf("Page to initialize is not in physical memory now\n");
//        return;
//    }
//    node_t *p = &memory[frame_no * PAGE_SIZE];
    node_t *p = get_page_address(page_no);
    p->head = 4095 << 1;

}

/**
 * Open the file to store evicted pages
 * @return an open file handler
 */
FILE *open_virtual_memory() {
    FILE *fp = fopen(FILENAME, "rb+");
    return fp;
}

/**
 * Allocate a long char array (This will be our physical memory), create the virtual memory file, initialize the page table
 */
void initialize() {
    //分配memory空间
    //初始化page table
    //初始化position时，默认position = page number
    initialized = 1;
    printf("Memory environment Initializing! \n");
    FILE *virtual_memory;
    if ((virtual_memory = fopen("virtual_memory", "wb")) == NULL) {
        printf("Fail to creating virtual memory file!\n");
    }
    fclose(virtual_memory);
    memory = (char *) malloc(sizeof(char) * PHYSICAL_MEMORY_SIZE);
    for (int i = 0; i < NUMBER_OF_PAGES; i++) {
        page_table[i].page_no = i;
        page_table[i].page_owner = -1;
        page_table[i].position = i;
        //是否需要在这里初始化page？
        //page_initialize(i);
    }
    printf("Memory environment complete! \n");
}

/**
 * Find a free page. There is three modes, kernel, user, and shared.
 * For user mode, the new page and current pages of that thread mustn't be conflicted
 * In other word, they can not be in identical slot
 * @param mode 0: kernel memory, 1: user memory, 2: shared memory
 * @param thread_id: This parameter is not required in kernel and shared mode
 * @return
 */
int find_a_free_page(int mode, my_pthread_t thread_id) {
    //kernel mode：遍历kernel区
    //User mode: 遍历user区
    int page_no = -1;
    if (mode == 0) {
        for (int base = 0; base < 3; base++) {
            for (int offset = 0; offset < NUMBER_OF_KERNEL_PAGES; offset++) {
                page_no = base * NUMBER_OF_FRAMES + offset;
                if (page_table[page_no].page_owner == -1) {
                    page_table[page_no].page_owner = 0;
                    return page_no;
                }
            }
        }
        printf("Error! Can not find a free kernel page! \n");
        return -1;
    } else if (mode == 1) {
        for (int base = 0; base < 3; base++) {
            for (int offset = 0 + NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES;
                 offset < NUMBER_OF_FRAMES; offset++) {
                page_no = base * NUMBER_OF_FRAMES + offset;
                if (page_table[page_no].page_owner == -1) {
                    int unconflicted = 1;
                    for (int i = 0; i <= 2; i++) {
                        if (page_table[i * NUMBER_OF_FRAMES + offset].page_owner == thread_id)
                            unconflicted = 0;
                        break;
                    }
                    if (unconflicted) {
                        page_table[page_no].page_owner = thread_id;
                        return page_no;
                    } else continue;
                }
            }
            printf("Error! Can not find a free user page! \n");
            return -1;
        }
    } else if (mode == 2) {
        for (int base = 0; base < 3; base++) {
            for (int offset = 0 + NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES;
                 offset < 0 + NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES; offset++) {
                if (page_table[page_no].page_owner == -1) {
                    page_table[page_no].page_owner = 0;
                    return page_no;
                }
            }
        }
        printf("Error! Can not find a free shared page! \n");
        return -1;
    }
}

/**
 * Copy data in memory1 to memory2
 * @param mode "swap": swap data in two areas, "cover": use data in memo1 to cover memo2
 */
void memory_copy(char *memo1, char *memo2, int size, char *mode) {
    //mode = "swap" or "cover"
    //cover: move memo1 to memo2 and cover memo2, memo2 is lost
    char temp;
    if (strcmp(mode, "swap") == 0) {
        for (int i = 0; i < size; i++) {
            temp = memo1[i];
            memo1[i] = memo2[i];
            memo2[i] = temp;
        }
    } else if (strcmp(mode, "cover") == 0) {
        for (int i = 0; i < size; i++)
            memo2[i] = memo1[i];
    }
    return;
}

/**
 * Swap two pages. Page_to_swapin is in the file, while page_to_evict in the physical memory(in the frame)
 * @param page_to_swapin
 * @param page_to_evict
 * @return
 */
int swap_page(int page_to_swapin, int page_to_evict) {
    //根据page number 找到page_to_evict所在的frame，以及page_to_swapin在文件中的offset
    //update position,交换两者
    int temp = page_table[page_to_swapin].position;
    // If this page already in, return immediately
    if (temp < NUMBER_OF_FRAMES) return 1;
    page_table[page_to_swapin].position = page_table[page_to_evict].position;
    page_table[page_to_evict].position = temp;
    FILE *fp = open_virtual_memory();
    char *address = get_page_address(page_to_evict);
    int offset = (page_table[page_to_swapin].position - NUMBER_OF_FRAMES) * PAGE_SIZE;
    char *temp_memo = (char *) malloc(sizeof(char) * PAGE_SIZE); //temp_memo stores data on the swapped in page
    if (page_table[page_to_swapin].page_owner != -1) {
        if (fseek(fp, offset, SEEK_SET) != 0) printf("Fseek fails \n");
        if (fread(temp_memo, sizeof(char), PAGE_SIZE, fp) != PAGE_SIZE) printf("Copy page fails \n");
    }
    if (fseek(fp, offset, SEEK_SET) != 0) printf("Fseek fails \n");
    if (fwrite(address, sizeof(char), PAGE_SIZE, fp) != PAGE_SIZE) printf("Evict page fails \n");
    fclose(fp);
    if (page_table[page_to_swapin].page_owner != -1) {
        memory_copy(temp_memo, address, PAGE_SIZE, "cover");
    } else page_initialize(page_to_swapin);
    return 0;
}

/**
 * Allocate memory chunk on the page. This function traverse the page, analyze the head integer in node_t.
 * Currently we adopt first-fit strategy and can be further optimized!
 * Splitting: If a chunk is big enough, allocate, then add a new head for the remaining part of the chunk
 * Coalescing: While traversing the page, merge the sequent free chunks. (Not implemented yet)
 * @param size : requested memory size
 * @param page_no : page number
 * @return
 */
void *allocate_on_page(int size, int page_no) {
    //在page上根据head数据分配空间
    char *start = &memory[page_no * PAGE_SIZE];
    char *end = &memory[page_no * PAGE_SIZE + PAGE_SIZE - 1];
    node_t *nodePtr = start;
    char *charPtr = start;
    int head = nodePtr->head, length = head >> 1, used = head & 1;
    while (charPtr < end && (used == 1 || length < size)) {
        charPtr += length + sizeof(node_t);
        nodePtr = charPtr;
        head = nodePtr->head;
        length = head >> 1;
        used = head & 1;
    }
    if (charPtr < end) {
        if (length > size + sizeof(node_t) + 1) {
            node_t *next = charPtr + size + sizeof(node_t);
            next->head = (length - size - sizeof(node_t)) << 1;
        }
        nodePtr->head += 1;
        return (void *) (charPtr + sizeof(node_t)); // might have problems?
    } else return NULL;
}

/**
 * Find the head based on the input parameter, and update the rightmost bit.
 * @param p : memory chunk to free
 */
void mydeallocate(char *p, char *file, int line, int thread_req) {
    node_t *nodePtr = p - sizeof(node_t);
    (nodePtr->head)--;
    return;
}

/**
 * Currently no replacement algorithm. Just pick the page in the required frame slot
 * @param page_to_swapin
 * @return
 */
int find_page_to_evict(int page_to_swapin) {
    // Didn't consider this: what if we input a page that already in the memory?
    int position = page_to_swapin % PAGE_SIZE;
    for (int i = 0; i < 3; i++) {
        int page_no = i * NUMBER_OF_FRAMES + position;
        if (page_table[page_no].position == position) return page_no;
    }
    return -1;
}

/**
 * Malloc free memory with requested size
 * @param x
 * @param file
 * @param line
 * @param thread_req
 * @return
 */
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
            for (int i = 0; i < MAX_PAGE_A_THREAD_CAN_USE; i++) {
                current_thread->memo_block->page[i] = -1;
            }
        }
        memory_control_block *memo_block = current_thread->memo_block;
        void *pointer = NULL;
        //First try to allocate space on the pages the thread already owns
        int i = 0;
        for (; memo_block->page[i] != -1 && i < MAX_PAGE_A_THREAD_CAN_USE; i++) {
            //Swap in this page first if it is not in the memory
            if (page_table[memo_block->page[i]].position >= NUMBER_OF_FRAMES) {
                int page_to_evict = find_page_to_evict(memo_block->page[i]);
                swap_page(memo_block->page[i], page_to_evict);
            }
            pointer = allocate_on_page(x, memo_block->page[i]);
            if (pointer != 0) break;
        }
        //If al currently owned pages are full, assign a new page. A new page definitely has enough space
        if (pointer == 0 && i < MAX_PAGE_A_THREAD_CAN_USE && memo_block->page[i] == -1) {
            // mode = user = 1
            memo_block->page[i] = find_a_free_page(1, current_thread->thread_id);
            page_initialize(memo_block->page[i]);
            // No page available
            if (memo_block->page[i] == -1) return NULL;
            // Swap in this page first if it is not in the memory
            if (page_table[memo_block->page[i]].position >= NUMBER_OF_FRAMES) {
                int page_to_evict = find_page_to_evict(memo_block->page[i]);
                swap_page(memo_block->page[i], page_to_evict);
            }
            pointer = allocate_on_page(x, memo_block->page[i]);
        } else {
            printf("This thread is using too many pages! \n");
            return NULL;
        }
        return pointer;
    } else if (thread_req = KERNEL_MODE) {
        if (x > PAGE_SIZE) {
            printf("Warning: kernel variable not allocated! \n");
            return NULL;
        }
        void *pointer = 0;
        int page_no = find_a_free_page(0, 0);
        page_initialize(page_no);
        pointer = allocate_on_page(x, page_no);
        if (pointer != 0) return pointer;
        else {
            printf("Warning: kernel space full! \n");
            return NULL;
        }
    }
}

void memory_maneger(thread_control_block *tcb) {
    memory_control_block *memo_block = tcb->memo_block;
    for (int i = 0; memo_block->page[i] != -1 && i < MAX_PAGE_A_THREAD_CAN_USE; i++) {
        int page_to_evict = find_page_to_evict(memo_block->page[i]);
        swap_page(memo_block->page[i], page_to_evict);
    }
    return;
}


