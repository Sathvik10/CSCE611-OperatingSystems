/*
    File: vm_pool.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/26/2010
    Update: 21/10/13

    Description: Management of the Virtual Memory Pool


*/

#ifndef _VM_POOL_H_ // include file only once
#define _VM_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */
#define GB *(0x1 << 30)
#define MB *(0x1 << 20)
#define KB *(0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"
#include "cont_frame_pool.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* Forward declaration of class PageTable */
/* We need this to break a circular include sequence. */
class PageTable;

/*--------------------------------------------------------------------------*/
/* V M  P o o l  */
/*--------------------------------------------------------------------------*/
/* Structure to hold free list and allocated list*/
struct node
{
    unsigned long base;
    unsigned long size;
};

typedef struct node Node;

class VMPool
{ /* Virtual Memory Pool */
private:
    /* -- DEFINE YOUR VIRTUAL MEMORY POOL DATA STRUCTURE(s) HERE. */

    // Variable to hold the base address
    unsigned long base_address;

    // Variable to hold the size
    unsigned long size;

    // Variable to hold the frame pool
    ContFramePool *frame_pool;

    // Variable to hold reference of page table
    PageTable *page_table;

    // Variable to hold reference to list that contains address of assigned memory slots
    Node *allocated_list;
    unsigned long allocated_list_count;

    // Variable to hold reference to list that contains address of free memory slots
    Node *free_list;
    unsigned long free_list_count;

    // Method to find free space within the memory pool
    unsigned long find_free_space(unsigned long size);

    // Method to delete a node in Linked List
    unsigned long delete_node(Node *head, unsigned long addr, int dir, unsigned long end);

public:
    VMPool(unsigned long _base_address,
           unsigned long _size,
           ContFramePool *_frame_pool,
           PageTable *_page_table);
    /* Initializes the data structures needed for the management of this
     * virtual-memory pool.
     * _base_address is the logical start address of the pool.
     * _size is the size of the pool in bytes.
     * _frame_pool points to the frame pool that provides the virtual
     * memory pool with physical memory frames.
     * _page_table points to the page table that maps the logical memory
     * references to physical addresses. */

    unsigned long allocate(unsigned long _size);
    /* Allocates a region of _size bytes of memory from the virtual
     * memory pool. If successful, returns the virtual address of the
     * start of the allocated region of memory. If fails, returns 0. */

    void release(unsigned long _start_address);
    /* Releases a region of previously allocated memory. The region
     * is identified by its start address, which was returned when the
     * region was allocated. */

    bool is_legitimate(unsigned long _address);
    /* Returns false if the address is not valid. An address is not valid
     * if it is not part of a region that is currently allocated. */
};

#endif
