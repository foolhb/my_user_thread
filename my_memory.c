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
#include <sys/mman.h>
#include <bits/siginfo.h>


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
#define FILENAME "swap_file"


void memory_manager();


void page_fault_handler(int signum, siginfo_t *si, void *unused);


/**
 * Page table entry to record page number, owner thread and position(position also indicates whether it is evicted)
 * Size: 12 Bytes
 */
typedef struct _page_table_entry {
//    int page_no;
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

char *memory;
struct sigaction *page_fault_sigaction;
page_table_entry page_table[NUMBER_OF_PAGES];

sigset_t signal_mask;
static int initialized = 0;

/**
 * Part I: Helper function for basic operations of pages, frames and page table, etc,
 * including get the start address of a page; given the page to swap in, find the page in the
 * physical memory which is in the required slot; initialize the usage record of a new page;
 */

/**
 * Get the starting address of a page
 * @param page_no
 * @return a pointer that points to the start address of this page
 */
void *get_start_address_of_page(int page_no) {
    int frame_no = page_table[page_no].position;
    if (frame_no > NUMBER_OF_FRAMES) {
        printf("Page to access not in physical memory now\n");
        return NULL;
    }
    void *p = &memory[frame_no * PAGE_SIZE];
    return p;
}

/**
 * Update the access permission of a page according to prot.
 * @param page_no
 * @param prot: forbid = PROT_NONE | PROT_WRITE; allow = PROT_READ
 * @return
 */
int mprotect_a_page(int page_no, int prot) {
    if (page_table[page_no].position >= NUMBER_OF_FRAMES) return -1;
    char *addr = get_start_address_of_page(page_no);
    int res = mprotect(addr, PAGE_SIZE, prot);
    return res;
}

/**
 * Update the access permission of a all user pages in the memory
 * @param prot: forbid = PROT_NONE | PROT_WRITE; allow = PROT_READ
 * @return
 */
int mprotect_all_pages(int prot) {
    char *addr = memory;
    addr += (NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES) * PAGE_SIZE;
    size_t len = (NUMBER_OF_FRAMES - NUMBER_OF_KERNEL_PAGES - NUMBER_OF_SHARED_PAGES) * PAGE_SIZE;
    int res = mprotect(addr, len, prot);
    return res;
}


/**
 * Given a pointer which points to a variable, get the page where the variable is located
 * @param p
 * @return
 */
int get_page_by_address(char *p) {
    char *start = &memory[0];
    return (int) ((p - start) / PAGE_SIZE);
}


/**
 * Initialize a new page: initialize a head to indicate that the whole page is empty
 * @param page_no
 */
void page_initialize(int page_no) {
    // The initial head will be 0001111111111110,
    // That means next memory chunk is 4095Kb, and is not used
    node_t *p = get_start_address_of_page(page_no);
    p->head = 4095 << 1;
}


/**
 * Open the swap file to store evicted pages
 * @return an open file handler
 */
FILE *open_virtual_memory() {
    FILE *fp = fopen(FILENAME, "rb+");
    return fp;
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
 * Copy data from memory1 to memory2.
 * @param memo1: points to a memory chunk with data in it
 * @param memo2: points to a memory chunk with data in it
 * @param size: Size of data that needs to be copied
 * @param mode "swap": swap data in two areas, "cover": use data in memory1 to cover memory2
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
 * Part II: Helper function for my_malloc APIs, including environment initialization,
 */


/**
 * Initialize library environment.
 * Pay attention: this library is not stateless and mustn't be interrupt
 */
void initialize() {
    initialized = 1;
    printf("Memory environment Initializing! \n");
    //initialize signal mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGVTALRM);
    //create the swap file;
    FILE *virtual_memory;
    if ((virtual_memory = fopen(FILENAME, "wb+")) == NULL) {
        printf("Fail to creating virtual memory file!\n");
    }
    fclose(virtual_memory);
    //allocate the physical memory, whose starting address is aligned to a page boundary;
    memory = (char *) memalign(PAGE_SIZE, sizeof(char) * PHYSICAL_MEMORY_SIZE);
    //initialize page table
    int i = 0;
    for (i = 0; i < NUMBER_OF_PAGES; i++) {
        page_table[i].page_owner = -1;
        page_table[i].position = i;
        page_table[i].next = -1;
    }
//    Register a page fault handler
    page_fault_sigaction = (struct sigaction *) malloc(sizeof(page_fault_sigaction));
    page_fault_sigaction->sa_handler = page_fault_handler;
    sigaction(SIGSEGV, page_fault_sigaction, NULL);
    printf("Memory environment complete! \n");
}

/**
 * Find a free page. There is three modes, kernel, user, and shared.
 * For user mode, the new page and current pages of that thread mustn't be conflicted
 * In other word, they can not be in identical slot
 * @param mode: 0 = kernel memory, 1 = user memory, 2 = shared memory
 * @param thread_id: This parameter is not required in kernel and shared mode
 * @return If new page found, return page number. Otherwise, return -1
 */
int find_free_pages(int mode, my_pthread_t thread_id) {
    int page_no;
    if (mode == KERNEL_MODE) {
        for (page_no = 0; page_no < NUMBER_OF_KERNEL_PAGES; page_no++) {
            if (page_table[page_no].page_owner == -1) {
                page_table[page_no].page_owner = 1;
                return page_no;
            }
        }
        printf("Can not find a free kernel page! \n");
        return -1;
    } else if (mode == SHARE_MODE) {
        for (page_no = NUMBER_OF_KERNEL_PAGES;
             page_no < NUMBER_OF_KERNEL_PAGES + NUMBER_OF_SHARED_PAGES; page_no++) {
            if (page_table[page_no].page_owner == -1) {
                page_table[page_no].page_owner = 1;
                return page_no;
            }
        }
        printf("Can not find a free shared page! \n");
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
    }
    return -1;
}


/**
 * Swap two pages. Page_to_swapin in the file; page_to_evict in the physical memory(in the frame)
 * @param page_to_swapin: page number of page to swapin
 * @param page_to_evict: page number of page to evict
 * @return
 */
int swap_page(int page_to_swapin, int page_to_evict) {
    if (page_table[page_to_swapin].position < NUMBER_OF_FRAMES) {
        // No need to swap. Already in the physical.
        return 1;
    }
    FILE *fp = open_virtual_memory();
    //Starting address of the frame
    char *address = NULL;
    //offset: location of the page to swap in. It is in the file now.
    int offset = (page_table[page_to_swapin].position - NUMBER_OF_FRAMES) * PAGE_SIZE;
    //temp_memo stores data on the page to swap in
    char *temp_memo = NULL;
    if (page_table[page_to_swapin].page_owner != -1) {
        temp_memo = (char *) malloc(sizeof(char) * PAGE_SIZE);
        if (fseek(fp, offset, SEEK_SET) != 0) {
//            printf("Fseek fails \n");
        }
        size_t read_size = fread(temp_memo, sizeof(char), PAGE_SIZE, fp);
        if (read_size != PAGE_SIZE) {
//            printf("Copy page fails \n");
        }
    }
    if (page_table[page_to_evict].page_owner != -1) {
        address = get_start_address_of_page(page_to_evict);
        if (fseek(fp, offset, SEEK_SET) != 0) {
//            printf("Fseek fails \n");
        }
        size_t write_size = fwrite(address, sizeof(char), PAGE_SIZE, fp);
        if (write_size != PAGE_SIZE) {
//            printf("Evict page fails \n");
        }
    }
    fclose(fp);
    if (page_table[page_to_swapin].page_owner != -1) {
        memory_copy(temp_memo, address, PAGE_SIZE, "cover");
        free(temp_memo);
    }
    //update page table entry
    int temp = page_table[page_to_swapin].position;
    page_table[page_to_swapin].position = page_table[page_to_evict].position;
    page_table[page_to_evict].position = temp;
    return 0;
}

/**
 * Allocate memory chunk on one page. This function traverse the page, analyze the head integer
 * in node_t. Currently we adopt first-fit strategy and can be further optimized!
 * Splitting: If a chunk is big enough, allocate, then update its head, and add a new head for
 *            the remaining part of the memory chunk
 * Coalescing: While traversing the page, merge the sequent small free chunks into big one.
 * @param size: requested memory size
 * @param page_no: page number
 * @return
 */
void *allocate_on_page(int size, int page_no) {
    char *start = &memory[page_no * PAGE_SIZE];
    char *end = &memory[page_no * PAGE_SIZE + PAGE_SIZE - 1];
    node_t *nodePtr = (node_t *) start;
    char *charPtr = start;
    int head = nodePtr->head, length = head >> 1, used = head & 1;
    if (length == 0) return NULL;
    while (charPtr < end && (used == 1 || length < size)) {
        charPtr += length + sizeof(node_t);
        nodePtr = (node_t *) charPtr;
        head = nodePtr->head;
        length = head >> 1;
        used = head & 1;
    }
    if (charPtr < end) {
        //add a new head for the remaining part of the memory chunk
        if (length > size + sizeof(node_t) + 1) {
            node_t *next = (node_t *) (charPtr + size + sizeof(node_t));
            next->head = (length - size - sizeof(node_t)) << 1;
        }
        // update the head of this allocated memory chunk
        nodePtr->head = (size << 1) + 1;
        return (void *) (charPtr + sizeof(node_t));
    } else return NULL;
}

/**
 * Given a frame number, check if there is one page related to this frame eligible for allocate_across_pages
 * @param frame_no
 * @param mode
 * @param thread_id
 * @return
 */
int eligible_for_across_pages(int frame_no, int mode, my_pthread_t thread_id) {
    int page_no = -1;
    if (mode == KERNEL_MODE || mode == SHARE_MODE) {
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
 * Allocate big memory chunk on continuous pages. This is for varaible larger than page size
 * @param size: requested memory size
 * @param mode: kernel = 0, user = 1, share = 2
 * @param tcb: In kernel or share mode, this parameter is not used
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
                    mprotect_a_page(new_page, PROT_READ | PROT_WRITE);
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

void memory_manager() {
    thread_control_block *tcb = get_current_running_thread();
    memory_control_block *memo_block = tcb->memo_block;
    if (memo_block == NULL) return;
    int i = 0;
    mprotect_all_pages(PROT_NONE);
    for (i = 0; i < memo_block->current_page; i++) {
        int page_no = memo_block->page[i];
        while (page_no != -1) {
            int page_to_evict = find_page_to_evict(page_no);
            mprotect_a_page(page_to_evict, PROT_WRITE | PROT_READ);
            swap_page(page_no, page_to_evict);
            page_no = page_table[page_no].next;
        }
    }
}

void page_fault_handler(int signum, siginfo_t *si, void *unused) {
    char *addr = si->si_addr;
}


