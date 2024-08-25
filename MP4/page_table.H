/*
    File: page_table.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 16/12/07

    Description: Basic Paging.

*/

#ifndef _page_table_H_ // include file only once
#define _page_table_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */
const int MAX_POOLS = 10;

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"
#include "exceptions.H"
#include "cont_frame_pool.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */
class VMPool;

/*--------------------------------------------------------------------------*/
/* P A G E - T A B L E  */
/*--------------------------------------------------------------------------*/

class PageTable
{

private:
    /* THESE MEMBERS ARE COMMON TO ENTIRE PAGING SUBSYSTEM */
    static PageTable *current_page_table;   /* pointer to currently loaded page table object */
    static unsigned int paging_enabled;     /* is paging turned on (i.e. are addresses logical)? */
    static ContFramePool *kernel_mem_pool;  /* Frame pool for the kernel memory */
    static ContFramePool *process_mem_pool; /* Frame pool for the process memory */
    static unsigned long shared_size;       /* size of shared address space */

    /* DATA FOR CURRENT PAGE TABLE */
    unsigned long *page_directory; /* where is page directory located? */
    unsigned long pde_address;     /* Holds the address value */
    VMPool *vmpool[MAX_POOLS];     /*Holds reference to the assigned VMPool*/
    int number_of_pools;           /*Holds number of registered pools*/

    /* Method to initiate a page table with a value and flag. It is used to
    set all the entries to zero when a page table is created. It can be used
    to set the mapping for the kernel space with multiplier = 4096*/
    void init_page_table(unsigned long *address, int mutliplier, int flag);

    /* Uses kernel frame pool manager to allocate a frame for page table*/
    unsigned long get_page_table_frame(int no_of_frames);

    /* Uses process frame pool manager to allocate a frame for the physical memory */
    unsigned long get_process_frame(int no_of_frames);

    /* Checks whether the given page table entry is valid or not based on the present bit*/
    bool is_valid_entry(unsigned long entry);

    /* Frees pages from a give base virtual address and number of pages */
    void free_page(unsigned long base_address, unsigned long number_of_pages);

public:
    static const unsigned int PAGE_SIZE = Machine::PAGE_SIZE;
    /* in bytes */
    static const unsigned int ENTRIES_PER_PAGE = Machine::PT_ENTRIES_PER_PAGE;
    /* in entries, duh! */

    static void init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size);
    /* Set the global parameters for the paging subsystem. */

    PageTable();
    /* Initializes a page table with a given location for the directory and the
       page table proper.
       NOTE: The PageTable object still needs to be stored somewhere!
       Probably it is best to have it on the stack, as there is no
       memory manager yet...
       NOTE2: It may also be simpler to create the first page table *before*
       paging has been enabled.
    */

    void load();
    /* Makes the given page table the current table. This must be done once during
       system startup and whenever the address space is switched (e.g. during
       process switching). */

    static void enable_paging();
    /* Enable paging on the CPU. Typically, a CPU start with paging disabled, and
       memory is accessed by addressing physical memory directly. After paging is
       enabled, memory is addressed logically. */

    static void handle_fault(REGS *_r);
    /* The page fault handler. */

    void register_pool(VMPool *_vm_pool);
    /* Register a virtual memory pool with the page table. */

    void free_page(unsigned long _page_no);
    /* If page is valid, release frame and mark page invalid. */

    /*Allocates memory and updates the page table entries for a given virtual address and size*/
    bool allocate(unsigned long virtual_address, unsigned long size);

    /* Get address of the page table for the given virtual address */
    unsigned long *get_page_table_addr(unsigned long address);
};

#endif
