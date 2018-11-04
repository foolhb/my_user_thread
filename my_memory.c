//
// Created by bh398 on 10/21/18.
//

#include "my_memory_t.h"
#include "my_pthread_t.h"
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>


#define THREADREQ 1
#define PHYSICAL_MEMORY_SIZE (8*1024*1024)
#define VIRTUAL_MEMORY_SIZE (3*8*1024*1024)
#define PAGE_SIZE (4*1024)
#define NUMBER_OF_PAGES (VIRTUAL_MEMORY_SIZE/PAGE_SIZE)
#define NUMBER_OF_FRAMES (PHYSICAL_MEMORY_SIZE/PAGE_SIZE)
#define MAX_PAGE_NUMBER_OF_A_THREAD 110
#define NUMBER_OF_KERNEL_PAGES 100
#define NUMBER_OF_SHARED_PAGES 4
#define KERNEL_MODE 0
#define USER_MODE 1
#define SHARE_MODE 2
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
    int next;
} page_table_entry;

/**
 * A simple integer number to record the memory usage inside a page.
 * Structure: length of next memory chunk | whether the chunk is used
 *            15 bits                       1 bit
 * Size: 4 Bytes
 */
typedef struct __node_t {
    int head;
} node_t;

static int initialized = 0;

char *memory;

page_table_entry page_table[NUMBER_OF_PAGES];
//page_table_entry *frame_table[NUMBER_OF_FRAMES];

sigset_t signal_mask;


/**
 * Part I: Helper function for basic operations of pages, frames and page table, etc, including get the start address of
 * a page; given the page to swap in, find the page in the physical memory which is in the required slot; initialize the
 * usage record of a new page;
 */

/**
 * Get the starting address of a page
 * @param page_no
 * @return a pointer that points to the start address
 */
void *get_start_address_of_page(int page_no) {
    //获取page的首地址，首先计算出frame number
    int frame_no = page_table[page_no].position;
    if (frame_no > NUMBER_OF_FRAMES) {
        printf("Page to access not in physical memory now\n");
        return NULL;
    }
    void *p = &memory[frame_no * PAGE_SIZE];
    return p;
}

/**
 * Given a pointer, get the page which it lies in
 * @param p
 * @return
 */
int get_page_by_address(char *p) {
    char *start = &memory[0];
    return (int) ((p - start) / PAGE_SIZE);
}


/**
 * Currently no replacement algorithm. Just pick the page in the required frame slot
 * @param page_to_swapin
 * @return
 */
int find_page_to_evict(int page_to_swapin) {
    // Didn't consider this: what if we input a page that already in the memory?
    int page_to_evict = -1;
    int position = page_to_swapin % NUMBER_OF_FRAMES;
    int i = 0;
    for (i = 0; i < 3; i++) {
        page_to_evict = i * NUMBER_OF_FRAMES + position;
        if (page_table[page_to_evict].position == position) break;
    }
    return page_to_evict;
}


/**
 * Initialize a new page: initial a head to indicate that the whole page is empty
 * @param page_no
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
    node_t *p = get_start_address_of_page(page_no);
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
 * Part II: Helper function for my_malloc APIs, including environment initialization,
 */


/**
 * Allocate a long char array (This will be our physical memory), create the virtual memory file, initialize the page table
 */
void initialize() {
    //分配memory空间
    //初始化page table
    //初始化position时，默认position = page number
    initialized = 1;
    printf("Memory environment Initializing! \n");
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGVTALRM);
    FILE *virtual_memory;
    if ((virtual_memory = fopen("virtual_memory", "wb+")) == NULL) {
        printf("Fail to creating virtual memory file!\n");
    }
    fclose(virtual_memory);
//    memory = (char *) malloc(sizeof(char) * PHYSICAL_MEMORY_SIZE);
    memory = (char *) memalign(PAGE_SIZE, sizeof(char) * PHYSICAL_MEMORY_SIZE);
    int i = 0;
    for (i = 0; i < NUMBER_OF_PAGES; i++) {
        page_table[i].page_no = i;
        page_table[i].page_owner = -1;
        page_table[i].position = i;
        page_table[i].next = -1;
        //是否需要在这里初始化page？
        //page_initialize(i);
    }
    printf("Memory environment complete! \n");
}

/**
 * Find a free page. There is three modes, kernel, user, and shared.
 * For user mode, the new page and current pages of that thread mustn't be conflicted
 * In other word, they can not be in identical slot
 * @param mode: 0 = kernel memory, 1 = user memory, 2 = shared memory
 * @param thread_id: This parameter is not required in kernel and shared mode
 * @return
 */
int find_free_pages(int mode, my_pthread_t thread_id) {
    //kernel mode：遍历kernel区
    //User mode: 遍历user区
    int page_no;
    if (mode == KERNEL_MODE) {
        for (page_no = 0; page_no < NUMBER_OF_KERNEL_PAGES; page_no++) {
            if (page_table[page_no].page_owner == -1) {
                page_table[page_no].page_owner = 1;
                return page_no;
            }
        }
        printf("Error! Can not find a free kernel page! \n");
        return -1;
    } else if (mode == USER_MODE) {
        int base = 0, offset = 0;
        for (base = 0; base < 3; base++) {
            for (offset = 0 + NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES;
                 offset < NUMBER_OF_FRAMES; offset++) {
                page_no = base * NUMBER_OF_FRAMES + offset;
                if (page_table[page_no].page_owner != -1) continue;
                int unconflicted = 1;
                int i = 0;
                for (i = 0; i < 3; i++) {
                    if (page_table[i * NUMBER_OF_FRAMES + offset].page_owner == thread_id) {
                        unconflicted = 0;
                        break;
                    }
                }
                if (unconflicted) {
                    page_table[page_no].page_owner = thread_id;
                    return page_no;
                }
            }
        }
        printf("Can not find a free user page!\n");
        return -1;
    } else if (mode == 2) {
        for (page_no = 0 + NUMBER_OF_KERNEL_PAGES;
             page_no < 0 + NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES; page_no++) {
            if (page_table[page_no].page_owner == -1) {
                page_table[page_no].page_owner = 0;
                return page_no;
            }
        }
        printf("Error! Can not find a free shared page! \n");
        return -1;
    }
    return -1;
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
        int i = 0;
        for (i = 0; i < size; i++) {
            temp = memo1[i];
            memo1[i] = memo2[i];
            memo2[i] = temp;
        }
    } else if (strcmp(mode, "cover") == 0) {
        int i = 0;
        for (i = 0; i < size; i++)
            memo2[i] = memo1[i];
    }
}

/**
 * Swap two pages. Page_to_swapin is in the file, while page_to_evict in the physical memory(in the frame)
 * @param page_to_swapin
 * @param page_to_evict
 * @return
 */
int swap_page(int page_to_swapin, int page_to_evict) {
    if (page_table[page_to_swapin].position < NUMBER_OF_FRAMES) {
        // No need to swap. Already in the frame.
        return 1;
    }
    FILE *fp = open_virtual_memory();
    char *address = NULL;
    int offset = (page_table[page_to_swapin].position - NUMBER_OF_FRAMES) * PAGE_SIZE;
//    int temp = page_table[page_to_swapin].position;
//    page_table[page_to_swapin].position = page_table[page_to_evict].position;
//    page_table[page_to_evict].position = temp;
    char *temp_memo = NULL; //temp_memo stores data on the swapped in page
    if (page_table[page_to_swapin].page_owner != -1) {
        temp_memo = (char *) malloc(sizeof(char) * PAGE_SIZE);
        if (fseek(fp, offset, SEEK_SET) != 0);
//            printf("Fseek fails \n");
        size_t read_size = fread(temp_memo, sizeof(char), PAGE_SIZE, fp);
        if (read_size != PAGE_SIZE);
//            printf("Copy page fails \n");
    }
    if (page_table[page_to_evict].page_owner != -1) {
        address = get_start_address_of_page(page_to_evict);
        if (fseek(fp, offset, SEEK_SET) != 0) printf("Fseek fails \n");
        size_t write_size = fwrite(address, sizeof(char), PAGE_SIZE, fp);
        if (write_size != PAGE_SIZE) printf("Evict page fails \n");
    }
    fclose(fp);
    if (page_table[page_to_swapin].page_owner != -1) {
        memory_copy(temp_memo, address, PAGE_SIZE, "cover");
        free(temp_memo);
    }
    int temp = page_table[page_to_swapin].position;
    page_table[page_to_swapin].position = page_table[page_to_evict].position;
    page_table[page_to_evict].position = temp;
    // Do we need to initialize page here?
//    else page_initialize(page_to_swapin);
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
    node_t *nodePtr = (node_t *) start;
    char *charPtr = start;
    int head = nodePtr->head, length = head >> 1, used = head & 1;
    // If this is a continuous page for across-pages variable, there will be no head
    if (length == 0) return NULL;
    while (charPtr < end && (used == 1 || length < size)) {
        charPtr += length + sizeof(node_t);
        nodePtr = (node_t *) charPtr;
        head = nodePtr->head;
        length = head >> 1;
        used = head & 1;
    }
    if (charPtr < end) {
        if (length > size + sizeof(node_t) + 1) {
            node_t *next = (node_t *) (charPtr + size + sizeof(node_t));
            next->head = (length - size - sizeof(node_t)) << 1;
        }
        nodePtr->head = (size << 1) + 1;
        return (void *) (charPtr + sizeof(node_t)); // might have problems?
    } else return NULL;
}

int eligible_for_across_pages(int frame_no, int mode, my_pthread_t thread_id) {
    int page_no = -1;
    if (mode == KERNEL_MODE) {
        if (page_table[frame_no].page_owner == -1) {
            page_no = frame_no;
        }
    } else if (mode == USER_MODE) {
        int base = 0;
        for (base = 0; base < 3; base++) {
            int i = NUMBER_OF_FRAMES * base + frame_no;
            if (page_table[i].page_owner == thread_id) {
                page_no = -1;
                break;
            }
            if (page_table[i].page_owner == -1 && (page_no == -1 || page_table[i].position == frame_no)) {
                page_no = i;
            }
        }
    }
    return page_no;
}

/**
 *
 * @param size
 * @param mode: kernel = 0, user = 1, share = 2
 * @param tcb: In kernel mode, this parameter is not used
 * @return
 */
void *allocate_across_pages(int size, int mode, thread_control_block *tcb) {
    int number_of_pages_needed = size / PAGE_SIZE + (size % PAGE_SIZE == 0 ? 0 : 1);
    int *continuous_pages = (int *) malloc(sizeof(int) * number_of_pages_needed);
    void *pointer = NULL;
    if (mode == KERNEL_MODE) {
        if (number_of_pages_needed > NUMBER_OF_KERNEL_PAGES) {
            printf("This variable is even larger than kernel physical memory\n");
//            return NULL;
        } else {
            int frame_no = 0, i = 0;
            while (frame_no < NUMBER_OF_KERNEL_PAGES && i < number_of_pages_needed) {
                int j = eligible_for_across_pages(frame_no, KERNEL_MODE, -1);
                if (j == -1) i = 0;
                else {
                    continuous_pages[i++] = j;
                    if (i == number_of_pages_needed) break;
                }
                frame_no++;
            }
            if (i != number_of_pages_needed) {
                printf("No enough continuous kernel pages!\n");
            } else {
                int j = 0;
                for (j = 0; j < number_of_pages_needed; j++) {
                    int page_to_evict = find_page_to_evict(continuous_pages[j]);
                    swap_page(continuous_pages[j], page_to_evict);
                    page_table[continuous_pages[j]].page_owner = 1;
                    page_table[continuous_pages[j]].next =
                            (j < number_of_pages_needed - 1 ? continuous_pages[j + 1] : -1);
                }
                pointer = get_start_address_of_page(continuous_pages[0]);
            }
        }
    } else if (mode == USER_MODE) {
        pthread_t thread_id = tcb->thread_id;
        if (number_of_pages_needed > NUMBER_OF_FRAMES - NUMBER_OF_SHARED_PAGES - NUMBER_OF_KERNEL_PAGES) {
            printf("This variable is even larger than user physical memory\n");
            free(continuous_pages);
            return NULL;
        }
        if (tcb->memo_block->current_page == MAX_PAGE_NUMBER_OF_A_THREAD) {
            printf("This thread already owns too much pages!\n");
            free(continuous_pages);
            return NULL;
        }
        int frame_no = 0 + NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES, i = 0;
        while (frame_no < NUMBER_OF_FRAMES && i < number_of_pages_needed) {
            int j = eligible_for_across_pages(frame_no, USER_MODE, thread_id);
            if (j == -1) i = 0;
            else {
                continuous_pages[i++] = j;
                if (i == number_of_pages_needed) break;
            }
            frame_no++;
        }
        if (i == number_of_pages_needed) {
            int j = 0;
            for (j = 0; j < number_of_pages_needed; j++) {
                int page_to_evict = find_page_to_evict(continuous_pages[j]);
                swap_page(continuous_pages[j], page_to_evict);
                page_table[continuous_pages[j]].page_owner = thread_id;
                page_table[continuous_pages[j]].next = (j < number_of_pages_needed - 1 ? continuous_pages[j + 1] : -1);
//            Shouldn't initialize page here!
//            page_initialize(continuous_pages[j]);
            }
            int k = (tcb->memo_block->current_page)++;
            tcb->memo_block->page[k] = continuous_pages[0];
            pointer = get_start_address_of_page(continuous_pages[0]);
        }
    }
    free(continuous_pages);
    return pointer;
}


/**
 * Malloc free memory with requested size
 * @param size
 * @param file
 * @param line
 * @param thread_req
 * @return
 */
void *myallocate(int size, char *file, int line, int thread_req) {
    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    if (initialized == 0) {
        initialize();
    }
//    sigprocmask(SIG_SETMASK, &signal_mask, NULL);
    void *pointer = NULL;
    if (thread_req == USER_MODE) {
        thread_control_block *current_thread = get_current_running_thread();
        //If it's a new TCB, initialize its memory control block.
        if (current_thread->memo_block == NULL) {
            current_thread->memo_block = (memory_control_block *) malloc(sizeof(memory_control_block));
//            current_thread->memo_block->page = (int *) malloc(sizeof(int) * MAX_PAGE_NUMBER_OF_A_THREAD);
            current_thread->memo_block->current_page = 0;
            int i = 0;
            for (i = 0; i < MAX_PAGE_NUMBER_OF_A_THREAD; i++) {
                current_thread->memo_block->page[i] = -1;
            }
        }
        if (size >= PAGE_SIZE) {
            pointer = allocate_across_pages(size, USER_MODE, current_thread);
            if (pointer == NULL) {
                printf("User variable too large to allocate across pages!\n");
            }
        } else {
            memory_control_block *memo_block = current_thread->memo_block;
            //First try to allocate space on the pages the thread already owns
            int i = 0;
//            for (; memo_block->page[i] != -1 && i < MAX_PAGE_NUMBER_OF_A_THREAD; i++) {
            for (; i < memo_block->current_page; i++) {
                //Swap in this page first if it is not in the memory
                int page_no = memo_block->page[i];
                if (page_table[page_no].next != -1) continue;
                if (page_table[page_no].position >= NUMBER_OF_FRAMES) {
                    int page_to_evict = find_page_to_evict(page_no);
                    swap_page(memo_block->page[i], page_to_evict);
                }
                pointer = allocate_on_page(size, page_no);
                if (pointer != NULL) {
                    break;
                }
            }
            //If al currently owned pages are full, assign a new page.
            //A new page definitely has enough space
            if (pointer == NULL) {
                if (i == MAX_PAGE_NUMBER_OF_A_THREAD) {
                    printf("This thread is using too many pages! \n");
                } else {
                    int new_page = find_free_pages(USER_MODE, current_thread->thread_id);
                    if (new_page != -1) {
                        memo_block->page[i] = new_page;
                        (memo_block->current_page)++;
                        int page_to_evict = find_page_to_evict(new_page);
                        swap_page(memo_block->page[i], page_to_evict);
                        page_initialize(new_page);
                        pointer = allocate_on_page(size, new_page);
                    }
                }
            }
        }
    } else if (thread_req == KERNEL_MODE) {
        if (size > PAGE_SIZE) {
            pointer = allocate_across_pages(size, KERNEL_MODE, NULL);
            if (pointer == NULL) {
                printf("Kernel variable too large to allocate across pages!\n");
            }
        } else {
            int page_no = find_free_pages(KERNEL_MODE, 0);
            if (page_no != -1) {
                page_initialize(page_no);
                pointer = allocate_on_page(size, page_no);
            } else {
                printf("No free kernel page npw!\n");
            }
        }
    }
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    return pointer;
}


/**
 * Find the head based on the input parameter, and update the rightmost bit.
 * @param p : memory chunk to free
 */
void mydeallocate(char *p, char *file, int line, int thread_req) {
    int page_no = get_page_by_address(p);
    if (page_table[page_no].next == -1) {
        //problem ?
        node_t *nodePtr = (node_t *) (p - sizeof(node_t));
        (nodePtr->head)--;
    } else {
        while (page_no != -1) {
            int temp = page_table[page_no].next;
            page_table[page_no].page_owner = -1;
            page_table[page_no].next = -1;
            page_no = temp;
        }
    }
}

void memory_manager(thread_control_block *tcb) {
    memory_control_block *memo_block = tcb->memo_block;
    if (memo_block == NULL) return;
    int i = 0;
    for (i = 0; i < memo_block->current_page; i++) {
        int page_no = memo_block->page[i];
        while (page_no != -1) {
            int page_to_evict = find_page_to_evict(page_no);
            swap_page(page_no, page_to_evict);
            page_no = page_table[page_no].next;
        }
    }
}


