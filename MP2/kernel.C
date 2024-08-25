/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 02/02/17


    This file has the main entry point to the operating system.

*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
/* Makes things easy to read */

#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))
/* Definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / (4 KB))
#define MEM_HOLE_SIZE ((1 MB) / (4 KB))
/* We have a 1 MB hole in physical memory starting at address 15 MB */

#define TEST_START_ADDR_PROC (4 MB)
#define TEST_START_ADDR_KERNEL (2 MB)
/* Used in the memory test below to generate sequences of memory references. */
/* One is for a sequence of memory references in the kernel space, and the   */
/* other for memory references in the process space. */

#define N_TEST_ALLOCATIONS 
/* Number of recursive allocations that we use to test.  */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"

#include "assert.H"
#include "cont_frame_pool.H"  /* The physical memory manager */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go);

void test_memory_custom(ContFramePool process_mem_pool, ContFramePool kernel_mem_pool);

/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    Console::init();

    /* -- INITIALIZE FRAME POOLS -- */

    /* ---- KERNEL POOL -- */
    
    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0);
    

    /* ---- PROCESS POOL -- */

    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);
    
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");

    /* -- TEST MEMORY ALLOCATOR */
    
    test_memory(&kernel_mem_pool, 32);
    test_memory_custom(process_mem_pool, kernel_mem_pool);

    /* ---- Add code here to test the frame pool implementation. */
    
    /* -- NOW LOOP FOREVER */
    Console::puts("Testing is DONE. We will do nothing forever\n");
    Console::puts("Feel free to turn off the machine now.\n");

    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go) {
    Console::puts(" alloc_to_go = "); Console::puti(_allocs_to_go); Console::puts("\n");
    if (_allocs_to_go > 0) {
        int n_frames = _allocs_to_go % 4 + 1;
        unsigned long frame = _pool->get_frames(n_frames);
        int * value_array = (int*)(frame * (4 KB));        
        for (int i = 0; i < (1 KB) * n_frames; i++) {
            value_array[i] = _allocs_to_go;
        }
        test_memory(_pool, _allocs_to_go - 1);
        for (int i = 0; i < (1 KB) * n_frames; i++) {
            if(value_array[i] != _allocs_to_go){
                Console::puts("MEMORY TEST FAILED. ERROR IN FRAME POOL\n");
                Console::puts("i ="); Console::puti(i);
                Console::puts("   v = "); Console::puti(value_array[i]); 
                Console::puts("   n ="); Console::puti(_allocs_to_go);
                Console::puts("\n");
                for(;;); 
            }
        }
        ContFramePool::release_frames(frame);
    }
}

void test_kernel_frame_allocation(ContFramePool kernel_mem_pool) {
    Console::puts("----- Testing allocation in kernel pool ----- \n");
    // Verify that allocation is successful and the returned frame number is not 0
    unsigned long frame_no = kernel_mem_pool.get_frames(1); 
    assert(frame_no != 0);
    // Release frames
    ContFramePool::release_frames(frame_no);
    Console::puts("Allocation test passed.\n");
}

void test_release_frame(ContFramePool kernel_mem_pool)
{
    Console::puts("----- Testing release frame in kernel pool ----- \n");
    unsigned long frame_no = kernel_mem_pool.get_frames(2);
    ContFramePool::release_frames(frame_no);
    unsigned long new_frame = kernel_mem_pool.get_frames(1);
    assert(frame_no == new_frame);
    Console::puts("Release frame test passed.\n");
}

void test_process_frame_allocation(ContFramePool process_mem_pool) {
    Console::puts("----- Testing allocation in process pool -----\n");
    // Similar to the kernel pool allocation, but using the process_mem_pool
    unsigned long frame_no = process_mem_pool.get_frames(3);
    assert(frame_no != 0); // Verify allocation success
    ContFramePool::release_frames(frame_no);
    Console::puts("Allocation test passed.\n");
}

void test_mark_inaccessible() {
    ContFramePool test_mem_pool(1,5,0);
    Console::puts("----- Testing inaccessibility marking -----\n");
    test_mem_pool.mark_inaccessible(1, 4);

    unsigned long frame = test_mem_pool.get_frames(10);
    assert(frame == 0); // Expect allocation to fail

    Console::puts("Inaccessibility test passed.\n");
}

void test_kernel_frames_allocation_limit(ContFramePool kernel_mem_pool) {
    Console::puts("-----  Testing kernel frame allocation limit -----\n");
    unsigned long frame_no = kernel_mem_pool.get_frames(KERNEL_POOL_SIZE + 1);
    assert(frame_no == 0); 
    Console::puts("Testing kernel frame allocation limit test passed.\n");
}

void test_needed_info_frames(ContFramePool process_mem_pool) {
    Console::puts("----- Testing needed_info_frames function -----\n");
    unsigned long info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);
    assert(info_frames == 1);
    Console::puts("Needed info frames test passed.\n");
}

void test_memory_custom(ContFramePool process_mem_pool, ContFramePool kernel_mem_pool){
    test_kernel_frame_allocation(kernel_mem_pool);
    test_process_frame_allocation(process_mem_pool);
    test_release_frame(kernel_mem_pool);
    test_mark_inaccessible();
    test_kernel_frames_allocation_limit(kernel_mem_pool);
    test_needed_info_frames(process_mem_pool);
}