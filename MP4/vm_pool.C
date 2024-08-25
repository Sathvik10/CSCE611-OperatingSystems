/*
 File: vm_pool.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

const unsigned long MAX_REGIONS = 512;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

/*Initialize data structures required to handle Virtual Address pool*/
VMPool::VMPool(unsigned long _base_address,
               unsigned long _size,
               ContFramePool *_frame_pool,
               PageTable *_page_table)
{
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    this->free_list = (Node *)(base_address);
    this->allocated_list = (Node *)(base_address + 4 KB - sizeof(Node));
    Console::puti((unsigned long)this->allocated_list);
    page_table->register_pool(this);
    Console::puts("Constructed VMPool object.\n");
}

/* Iterate a given linked list and deleted the node whose base address matches the given addr */
unsigned long VMPool::delete_node(Node *head, unsigned long addr, int dir, unsigned long end)
{
    int i = 0;
    unsigned long _size = 0;
    for (; i < end; i++)
    {
        if (head[i * dir].base == addr)
        {
            _size = head[i * dir].size;
            break;
        }
    }

    for (int j = i; j < end - 1; j++)
    {
        head[j * dir].base = head[(j + 1) * dir].base;
        head[j * dir].size = head[(j + 1) * dir].size;
    }
    return _size;
}

/* Method to find free space within the memory pool */
unsigned long VMPool::find_free_space(unsigned long _size)
{
    if (_size == 0 || this->free_list_count == 0)
    {
        return 0;
    }

    /* Iterate the free list and find the region with size bigger than requested size */
    for (int i = 0; i < free_list_count; i++)
    {
        /* If requested size is lesser than entry size, decrease the size of the free list entry */
        /* and add a new entry in allocated list */
        if (free_list[i].size > _size)
        {
            unsigned long addr = free_list[i].base;
            free_list[i].base = addr + _size;
            allocated_list[-1 * (allocated_list_count)].base = addr;
            allocated_list[-1 * (allocated_list_count)].size = _size;
            allocated_list_count++;
            return addr;
        }
        /* If requested size is equal to entry size, delete the free list entry */
        /* and add a new entry in allocated list */
        else if (free_list[i].size == _size)
        {
            unsigned long addr = free_list[i].base;
            delete_node(free_list, i, 1, free_list_count);
            free_list_count--;
            allocated_list[-1 * (allocated_list_count)].base = addr;
            allocated_list[-1 * (allocated_list_count)].size = _size;
            allocated_list_count++;
            return addr;
        }
    }
    return 0;
}

/* Allocates a region of virtual space*/
unsigned long VMPool::allocate(unsigned long _size)
{
    /* Inititalize the free list and allocated regions for the first time*/
    if (free_list_count == 0 && allocated_list_count == 0)
    {
        this->free_list[0].base = base_address + 4 KB;
        this->free_list->size = size - 4 KB;
        free_list_count++;
    }

    /* Total size of metadata is 4 KB. Check whether the free list*/
    /* and allocated regions acquire more than 4 KB. */
    if (free_list_count + allocated_list_count >= MAX_REGIONS)
    {
        Console::puts("Unable to allocate due to less space in metadata storage.\n");
        return 0;
    }
    unsigned long va = find_free_space(_size);

    /* If unable to find the virtual address, mark as failure to allocate the requested size */
    if (va == 0)
    {
        Console::puts("No Free space in VMPool. Failed to Allocate memory.\n");
        return 0;
    }

    /* For the allocate virtual address, map the physical address using page table */
    if (page_table->allocate(va, _size))
    {
        Console::puts("Allocated region of memory. Starting from: ");
        Console::puti(va);
        Console::puts("\n");
        return va;
    }

    /* If mapping address in page table fails, recapture the regions and add entries to free list */
    delete_node(allocated_list, va, -1, allocated_list_count);
    allocated_list_count--;
    free_list[free_list_count].base = va;
    free_list[free_list_count].size = _size;
    free_list_count++;
    Console::puts("Failed to Allocate region of memory.\n");
    return 0;
}

/* Releases a region of previously allocated memory. The region
 * is identified by its start address, which was returned when the
 * region was allocated. */
void VMPool::release(unsigned long _start_address)
{
    /* Delete the entry in the allocated list. */
    unsigned long _size = delete_node(allocated_list, _start_address, -1, allocated_list_count);
    allocated_list_count--;

    unsigned long frames = ContFramePool::needed_info_frames(_size);
    unsigned long free_address = _start_address;

    /*For each frame invalidate the page table entry*/
    for (int i = 0; i < frames; i++)
    {
        page_table->free_page(free_address);
        free_address += Machine::PAGE_SIZE;
    }

    free_list[free_list_count].base = _start_address;
    free_list[free_list_count].size = _size;
    Console::puts("Released region of memory.\n");
}

/* Returns false if the address is not valid. An address is not valid
 * if it is not part of a region that is currently allocated. */
bool VMPool::is_legitimate(unsigned long _address)
{
    if (_address >= base_address && _address < (base_address + size))
    {
        return true;
    }
    return false;
}