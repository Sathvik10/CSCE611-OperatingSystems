#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)

const int table_entries_per_page = 1024;

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
   if(kernel_mem_pool == NULL || process_mem_pool == NULL)
      assert(false);
   
   unsigned long pd_frame = kernel_mem_pool->get_frames(1);
   if(pd_frame == 0)
   {
      Console::puts("Page Table failed. Unable to get frame in kernel space\n");
      return;
   }

   // Calculate the address of page directory
   this->page_directory = (unsigned long *) (pd_frame * 4 KB);
  
   // Store the value in a variable
   this->pde_address = (pd_frame * 4 KB);

   // Number of shared pages
   int shared_pages_size = shared_size / (4 KB);

   // Number of page table frames required to store shared pages
   unsigned long n_info_frames = (shared_pages_size * 4 )/(4 KB);

   // Frame Number of page table holding kernel
   unsigned long kernel_page_table = kernel_mem_pool->get_frames(n_info_frames);

   unsigned long * page_table = (unsigned long *)(kernel_page_table * 4 KB);

   // Mapping physical address to virtual address in kernel space
   init_page_table(page_table, 4096, 3);

   // Zeroing the page directory
   init_page_table(this->page_directory, 0, 2);

   this->page_directory[0] = (kernel_page_table * 4 KB);
   this->page_directory[0] = this->page_directory[0] | 3;

   Console::puts("Constructed Page Table object\n");
}

void PageTable::init_page_table(unsigned long * address, int mutliplier, int flag)
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


void PageTable::handle_fault(REGS * _r)
{
   // Obtain Fault Address from CR2
   unsigned long fault_addr = read_cr2();

   // Console::puti(fault_addr >> 22); Console::puts(" ");
   // Console::puti((fault_addr >> 12) & 0X3FF); Console::puts(" ");
   // Console::puti((fault_addr) & 0XFFF); Console::puts(" ");

   PageTable* cpt = current_page_table;

   // Get the values for paging |10|10|12|
   unsigned long pd_entry = fault_addr >> 22;
   unsigned long pt_entry = (fault_addr >> 12) & 0X3FF;

   // Holds the page table for faulted adress
   unsigned long * page_table = NULL;

   // If the Page Directory entry is invalid, create a frame from the kernel frame for page table page and update it accordingly
   if(!cpt->is_valid_entry(cpt->page_directory[pd_entry]))
   {
      unsigned long page_address = cpt->get_page_table_frame(1);
      page_table = (unsigned long *) page_address;
      cpt->init_page_table(page_table, 0, 2);
      page_address = page_address | 3;
      cpt->page_directory[pd_entry] = page_address;
   }
   else // If the page director entry is valid, get the frame address of the page table
   {
      unsigned long page_address = cpt->page_directory[pd_entry];
      page_address = page_address >> 12;
      page_table = (unsigned long *) (page_address << 12);
   }

   // Create a frame (physical address) from the process frame pool and update the page table entry
   unsigned long process_page_address = cpt->get_process_frame(1);
   process_page_address = process_page_address | 7;

   page_table[pt_entry] = process_page_address;
   Console::puts("handled page fault\n");
}

