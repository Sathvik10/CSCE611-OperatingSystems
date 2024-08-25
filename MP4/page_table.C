#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"
#include "vm_pool.H"

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)

const int table_entries_per_page = 1024;

PageTable *PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = NULL;
ContFramePool *PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    if (kernel_mem_pool == NULL || process_mem_pool == NULL)
        assert(false);

    number_of_pools = 0;

    unsigned long pd_frame = kernel_mem_pool->get_frames(1);
    if (pd_frame == 0)
    {
        Console::puts("Page Table failed. Unable to get frame in Process space\n");
        return;
    }

    // Store the value in a variable
    this->pde_address = (pd_frame * 4 KB);

    // Calculate the address of page directory
    this->page_directory = (unsigned long *)(this->pde_address);

    // Number of shared pages
    int shared_pages_size = shared_size / (4 KB);

    // Number of page table frames required to store shared pages
    unsigned long n_info_frames = (shared_pages_size * 4) / (4 KB);

    // Frame Number of page table holding kernel
    unsigned long kernel_page_table = process_mem_pool->get_frames(n_info_frames);

    unsigned long *page_table = (unsigned long *)(kernel_page_table * 4 KB);

    // Mapping physical address to virtual address in kernel space
    init_page_table(page_table, 4096, 3);

    // Zeroing the page directory
    init_page_table(this->page_directory, 0, 2);

    this->page_directory[0] = (kernel_page_table * 4 KB);
    this->page_directory[0] = this->page_directory[0] | 3;
    this->page_directory[1023] = this->pde_address | 3;

    Console::puts("Constructed Page Table object\n");
}

void PageTable::init_page_table(unsigned long *address, int mutliplier, int flag)
{
    for (int i = 0; i < 1024; i++)
    {
        address[i] = (i * mutliplier) | flag;
    }
}

void PageTable::load()
{
    current_page_table = this;
    write_cr3(this->pde_address);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    paging_enabled = 1;
    write_cr0(read_cr0() | 0x80000000);
    Console::puts("Enabled paging\n");
}

unsigned long PageTable::get_page_table_frame(int no_of_frames)
{
    unsigned long frame_no = kernel_mem_pool->get_frames(no_of_frames);
    return frame_no * 4 KB;
}

unsigned long PageTable::get_process_frame(int no_of_frames)
{
    unsigned long frame_no = process_mem_pool->get_frames(no_of_frames);
    return frame_no * 4 KB;
}

bool PageTable::is_valid_entry(unsigned long entry)
{
    return entry & 1;
}

void PageTable::handle_fault(REGS *_r)
{
    // Obtain Fault Address from CR2
    unsigned long fault_addr = read_cr2();

    PageTable *cpt = current_page_table;

    if (cpt->number_of_pools > 0)
    {
        bool valid_addr = false;
        for (int i = 0; i < cpt->number_of_pools; i++)
        {
            if (cpt->vmpool[i]->is_legitimate(fault_addr))
            {
                valid_addr = true;
                break;
            }
        }
        if (!valid_addr)
        {
            return;
        }
    }

    // Get the values for paging |10|10|12|
    unsigned long pd_entry = fault_addr >> 22;
    unsigned long pt_entry = (fault_addr >> 12) & 0X3FF;

    // Holds the page table for faulted adress
    unsigned long *page_table = NULL;

    // If the Page Directory entry is invalid, create a frame from the kernel frame for page table page and update it accordingly
    if (!cpt->is_valid_entry(cpt->page_directory[pd_entry]))
    {
        unsigned long page_address = cpt->get_page_table_frame(1);
        page_table = (unsigned long *)page_address;
        cpt->init_page_table(page_table, 0, 2);
        page_address = page_address | 3;
        cpt->page_directory[pd_entry] = page_address;
    }
    else // If the page director entry is valid, get the frame address of the page table
    {
        unsigned long page_address = cpt->page_directory[pd_entry];
        page_address = page_address >> 12;
        page_table = (unsigned long *)(page_address << 12);
    }

    // Create a frame (physical address) from the process frame pool and update the page table entry
    unsigned long process_page_address = cpt->get_process_frame(1);
    process_page_address = process_page_address | 7;

    page_table[pt_entry] = process_page_address;
    Console::puts("Handled page fault\n");
}

/* Get address of the page table for the given virtual address */
unsigned long *PageTable::get_page_table_addr(unsigned long address)
{
    // Get the values for paging |10|10|12|
    unsigned long pd_entry = address >> 22;

    // Holds the page table for faulted adress
    unsigned long *page_table = NULL;

    if (!is_valid_entry(page_directory[pd_entry]))
    {
        unsigned long page_address = get_page_table_frame(1);
        if (page_address == 0)
        {
            return nullptr;
        }

        page_table = (unsigned long *)page_address;
        init_page_table(page_table, 0, 2);
        page_address = page_address | 3;
        page_directory[pd_entry] = page_address;
    }
    else // If the page director entry is valid, get the frame address of the page table
    {
        unsigned long page_address = page_directory[pd_entry];
        page_address = page_address >> 12;
        page_table = (unsigned long *)(page_address << 12);
    }

    return page_table;
}

/* Free the number of pages from the given base address */
void PageTable::free_page(unsigned long base_address, unsigned long number_of_pages)
{
    if (number_of_pages <= 0)
        return;

    /* Find the base address of each page, invalidate the page table entry */
    for (int i = 0; i < 1; i++)
    {
        unsigned long base_address_va = base_address + i * 4 KB;
        free_page(base_address_va);
    }
}

// Allocates memory and updates the page table entries for a given virtual address and size
bool PageTable::allocate(unsigned long virtual_address, unsigned long size)
{
    /* Calculate number of frames required for the given size */
    unsigned long no_of_frames = ContFramePool::needed_info_frames(size);
    unsigned long base_address_va = virtual_address;

    /* Get frames from the process frame  pool*/
    unsigned long physical_base_address = get_process_frame(no_of_frames);

    if (physical_base_address == 0)
    {
        Console::puts("Unable to get physical frame from the process frame pool");
        return false;
    }

    /* For each frame, map virtual address to the physical address */
    for (int i = 0; i < no_of_frames; i++)
    {
        base_address_va = virtual_address + i * 4 KB;
        unsigned long pt_entry = (base_address_va >> 12) & 0X3FF;

        unsigned long *page_table_addr = get_page_table_addr(base_address_va);

        /* If the page table address is null, then return false*/
        if (page_table_addr == nullptr)
        {
            // Change this
            free_page(virtual_address, i);
            return false;
        }

        /* Update the page table entry */
        unsigned long physical_page_address = physical_base_address + i * 4 KB;
        physical_page_address = physical_page_address | 7;
        page_table_addr[pt_entry] = physical_page_address;
    }
    return true;
}

/* Register the pool in the given page table*/
void PageTable::register_pool(VMPool *_vm_pool)
{
    if (number_of_pools == MAX_POOLS)
    {
        Console::puts("System restricts registering more than 10 pools");
        assert(false);
    }
    this->vmpool[number_of_pools++] = _vm_pool;
    Console::puts("registered VM pool\n");
}

/* Frees pages from a give base virtual address and number of pages */
void PageTable::free_page(unsigned long _page_no)
{
    if (_page_no <= 0)
        return;

    unsigned long base_address = _page_no;
    /* Find the base address of each page, invalidate the page table entry */

    unsigned long base_address_va = base_address;
    unsigned long pt_entry = (base_address_va >> 12) & 0X3FF;
    unsigned long *page_table_addr = get_page_table_addr(base_address_va);
    unsigned long physical_page_address = page_table_addr[pt_entry];

    physical_page_address = physical_page_address >> 1;
    physical_page_address = physical_page_address << 1;
    page_table_addr[pt_entry] = physical_page_address;

    process_mem_pool->release_frames(physical_page_address / Machine::PAGE_SIZE);

    /* Reload the CR3 register and invalidate TLB entries */
    load();
    Console::puts("Freed page\n");
}
