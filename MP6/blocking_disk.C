/*
     File        : blocking_disk.c

     Author      :
     Modified    :

     Description :

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"

extern Scheduler *SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

/* Constructor to initialiaze the variables and other data structures*/
BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size)
    : SimpleDisk(_disk_id, _size)
{
  /* Initially the blocking queue is empty.*/
  head = nullptr;
  tail = nullptr;

  /* The queue is empty.*/
  no_of_disk_threads = 0;

  /* set the lock to -1 to indicate the thread is important..*/
  current_thread = -1;

  /* Variable to control the delay*/
  delay = 0;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

/*  This function is used by the scheduler to check whether there are any disc threads
    ready to execute. */
bool BlockingDisk::is_ready()
{
  /*  If there are threads in the queue and no threads holds the lock.
      Return true to execute the threads in the waiting queue.*/
  if (no_of_disk_threads > 0 && current_thread == -1)
  {
    /* Code to induce delay.*/
    if (delay == 0)
    {
      delay = 1;
      return false;
    }
    delay = 0;
    Console::puts("No Current operation. Next Operation is ready.\n");
    return true;
  }
  /* If disc is ready for disk to transfer data, execute the blocked thread.*/
  else if (no_of_disk_threads > 0 && SimpleDisk::is_ready())
  {
    /* Code to induce delay.*/
    if (delay == 0)
    {
      delay = 1;
      return false;
    }
    delay = 0;
    Console::puts("Disk operation is ready to continue.\n");
    return true;
  }

  /* In all other cases return false.*/
  return false;
}

void BlockingDisk::add_disk_thread_to_front(Thread *_thread)
{
  DiskThreadList *_node = new DiskThreadList;
  _node->thread = _thread;
  this->no_of_disk_threads++;

  if (head == nullptr)
  {
    _node->next = nullptr;
    head = tail = _node;
    return;
  }

  _node->next = head;
  head = _node;
}

/* Function to add a thread to a list of blocked threads.*/
void BlockingDisk::add_disk_thread(Thread *_thread)
{
  DiskThreadList *_node = new DiskThreadList;
  _node->thread = _thread;
  _node->next = nullptr;
  this->no_of_disk_threads++;

  /* Addding node to the empty list.*/
  if (head == nullptr)
  {
    head = _node;
    tail = _node;
    return;
  }

  // Adding node to the end of the queue
  tail->next = _node;
  tail = tail->next;
  return;
}

/* Get the latest thread in the queue.*/
Thread *BlockingDisk::get_disk_thread()
{
  DiskThreadList *node = head;
  head = head->next;

  Thread *thread = node->thread;
  delete node;

  this->no_of_disk_threads--;
  return thread;
}

/*  Verifies if other threads are operating on the disk.
    If yes, waits by yeilding the thread. */
void BlockingDisk::check_other_disk_operation()
{
  if (current_thread != -1)
  {
    Console::puts("Yeilding the thread as other disk operation in progress.\n");
    add_disk_thread(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
  Console::puts("No other disk operation.\n");
}

/*  Yeilds a thread, if the disk is not ready to transfer the data.*/
void BlockingDisk::wait_until_ready()
{
  if (!SimpleDisk::is_ready())
  {
    Console::puts("Yeilding the thread as disk is not ready.\n");
    add_disk_thread_to_front(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
  Console::puts("Continuing operation.\n");
}

/* Sets the lock varible to current thread ID*/
void BlockingDisk::set_current_thread()
{
  // if (Machine::interrupts_enabled())
  //   Machine::disable_interrupts();

  current_thread = Thread::CurrentThread()->ThreadId();

  // if (!Machine::interrupts_enabled())
  //   Machine::enable_interrupts();
}

/* Releases the lock.*/
void BlockingDisk::clear_current_thread()
{
  // if (Machine::interrupts_enabled())
  //   Machine::disable_interrupts();

  current_thread = -1;

  // if (!Machine::interrupts_enabled())
  //   Machine::enable_interrupts();
}

/*  The execution is as follows:
    1. Check if other threads are performing an operation.
    2. Get the lock by updating the current thread value.
    3. Issue operation.
    4. Waiting until the disk is ready.
    5. Perform the operation.
    6. Release the lock, by setting the variable to -1.*/
void BlockingDisk::read(unsigned long _block_no, unsigned char *_buf)
{
  // SimpleDisk::read(_block_no, _buf);

  check_other_disk_operation();

  set_current_thread();

  issue_operation(DISK_OPERATION::READ, _block_no);

  wait_until_ready();

  /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++)
  {
    tmpw = Machine::inportw(0x1F0);
    _buf[i * 2] = (unsigned char)tmpw;
    _buf[i * 2 + 1] = (unsigned char)(tmpw >> 8);
  }

  clear_current_thread();

  Console::puts("read::Read operation is complete.\n");
}

/* Functionality similar to read.*/
void BlockingDisk::write(unsigned long _block_no, unsigned char *_buf)
{
  check_other_disk_operation();

  set_current_thread();

  issue_operation(DISK_OPERATION::WRITE, _block_no);

  wait_until_ready();

  /* write data to port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++)
  {
    tmpw = _buf[2 * i] | (_buf[2 * i + 1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }

  clear_current_thread();
  Console::puts("write::Write operation is complete.\n");
}

MirroredDisk::MirroredDisk(unsigned int size) : SimpleDisk(DISK_ID::MASTER, size)
{
}
