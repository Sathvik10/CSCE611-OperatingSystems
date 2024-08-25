/*
 File: scheduler.C

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

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

extern BlockingDisk *SYSTEM_DISK;
/* Initialize data structures required for scheduler functionalities.
  Head points to the head the of ready queue.
  Tail points to the end of the ready queue.
 */
Scheduler::Scheduler()
{
    head = nullptr;
    tail = nullptr;
    Console::puts("Constructed Scheduler.\n");
}

/* Function will add a new thread to the ready queue and returns a
   reference to the new node. */
void Scheduler::addNode(Thread *_thread)
{
    Node *_node = new Node;
    _node->thread = _thread;
    _node->next = nullptr;

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

void Scheduler::add(Thread *_thread)
{
    addNode(_thread);
}

/* Yeild CPU to the next thread.*/
void Scheduler::yield()
{
    /* Disable Interrupts until you dispatch a new thread.*/
    // if (Machine::interrupts_enabled())
    // {
    //     Machine::disable_interrupts();
    // }

    /* Check if there are any blocked threads waiting for the disk operation.*/
    if (SYSTEM_DISK->is_ready())
    {
        /* Get  the latest threadn and yield.*/
        Thread *next_thread = SYSTEM_DISK->get_disk_thread();
        Console::puts("Yeilding Thread To Blocking Disk Thread [");
        Console::puti(next_thread->ThreadId());
        Console::puts("]\n");

        // /* Enable the interrupts before dispatching the thread.*/
        // if (!Machine::interrupts_enabled())
        // {
        //     Machine::enable_interrupts();
        // }

        Thread::dispatch_to(next_thread);
        return;
    }

    /* If the ready queue is empty stop the execution*/
    if (head == nullptr)
    {
        assert(false);
    }

    /* Get the node at the starting of ready queue.*/
    Node *next = head;
    head = head->next;

    Thread *next_thread = next->thread;
    delete next;

    Console::puts("Yeilding Thread To [");
    Console::puti(next_thread->ThreadId());
    Console::puts("]\n");

    // /* Enable the interrupts before dispatching the thread.*/
    // if (!Machine::interrupts_enabled())
    // {
    //     Machine::enable_interrupts();
    // }

    /* Dispatch the next thread.*/
    Thread::dispatch_to(next_thread);
}

void Scheduler::resume(Thread *_thread)
{
    /* Add the node to ready queue.*/
    addNode(_thread);

    /* Hold the threadId.*/
    last_thread_id = _thread->ThreadId();
}

/* Because of the way the scheduler is implemented, there is no requirement of this method.
  Every thread that is dispacted from the scheduler is considered as terminate node
  and hence removed from the queue. */
void Scheduler::terminate(Thread *_thread)
{
}

/* Function of Ideal Thread */
void idealThreadFunc()
{
    Console::puts("Ideal Thread Starting\n");
    for (int i = 0; i < 100; i++)
    {
        for (int j = 0; j < 100000; j++)
        {
        }
    }
}
