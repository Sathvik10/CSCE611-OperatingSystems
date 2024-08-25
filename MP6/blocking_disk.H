/*
     File        : blocking_disk.H

     Author      :

     Date        :
     Description :

*/

#ifndef _BLOCKING_DISK_H_
#define _BLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

/* A data structure to maintain the ready queue [Linked List].*/
typedef struct disk_thread
{
   Thread *thread;
   disk_thread *next;
} DiskThreadList;

class BlockingDisk : public SimpleDisk
{

private:
   /* A variable will hold the starting node of the linked list.*/
   DiskThreadList *head;

   /* The variable will hold the last node of the linked list. This is will
      enable to append new threads on to the ready queue in O(n) */
   DiskThreadList *tail;

   /* Variable to hold the number of threads requested for the disk operation.*/
   unsigned int no_of_disk_threads;

   /* Variable to get the lock for disk operation.*/
   unsigned int current_thread;

   /* Variable to induce delay in disk operation.*/
   unsigned int delay;

   /* Function to add  a disk to a list of blocked threads.*/
   void add_disk_thread(Thread *thread);

   /* Function to add a disk to a list of blocked threads to front.*/
   void add_disk_thread_to_front(Thread *thread);

   /* Function to get the lock current_thread. The method will set the
      current_thread variable to thread Id of current thread.*/
   void set_current_thread();

   /* Release the lock for other threads to perform the disk operation.*/
   void clear_current_thread();

   /* Before the disk operation, check whether other threads are accessing the disk.
      If yes, wait in the queue for the turn.*/
   void check_other_disk_operation();

   /* Function to yeild the thread, if disk is not ready to continue the operation.*/
   virtual void wait_until_ready();

public:
   BlockingDisk(DISK_ID _disk_id, unsigned int _size);
   /* Creates a BlockingDisk device with the given size connected to the
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness.
      In a real system, we would infer this information from the
      disk controller. */

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char *_buf);
   /* Reads 512 Bytes from the given block of the disk and copies them
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char *_buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

   virtual bool is_ready();
   /* Return true if disk is ready to transfer data from/to disk, false otherwise. Used by the scheduler*/

   /* Get the latest thread in the queue. Used by the scheduler*/
   Thread *get_disk_thread();
};

class MirroredDisk : SimpleDisk
{
public:
   MirroredDisk(unsigned int size);
};

#endif
