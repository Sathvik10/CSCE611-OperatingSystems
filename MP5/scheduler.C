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
  if (Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
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

  /* Enable the interrupts before dispatching the thread.*/
  if (!Machine::interrupts_enabled())
  {
    Machine::enable_interrupts();
  }

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

/* Overriding the Yield function.*/
void RRSchedular::yield()
{
  /* EOI message to master interrupt controller*/
  Machine::outportb(0X20, 0X20);
  /* Disable Interrupts until you dispatch a new thread.*/
  if (Machine::interrupts_enabled())
  {
    Machine::disable_interrupts();
  }

  /* Resetting the ticks = 0.*/
  ticks = 0;
  Thread *next_thread;
  /* If the ready queue is empty. Initiate an ideal thread. */
  if (head == nullptr)
  {
    char *idealStack = new char[1024];
    Thread *idealThread = new Thread(idealThreadFunc, idealStack, 1024);
    Console::puts("Yeilding thread to ideal thread\n");
    next_thread = idealThread;
  }
  else
  {
    /* Find the next node in the ready queue.*/
    Node *next = head;
    head = head->next;
    next_thread = next->thread;
    delete next;

    Console::puts("RR Scheduler Yeilding Thread To [");
    Console::puti(next_thread->ThreadId());
    Console::puts("]\n");
  }

  /* Enable the interrupts before dispatching the thread.*/
  if (!Machine::interrupts_enabled())
  {
    Machine::enable_interrupts();
  }
  /* Dispatch the next thread.*/
  Thread::dispatch_to(next_thread);
}

/*Overriden function to handle the iterrupts*/
void RRSchedular::handle_interrupt(REGS *_r)
{
  ticks++;

  /* Whenever a time limit is over, we update counter accordingly. */
  if (ticks >= hz)
  {
    seconds++;
    ticks = 0;
    Console::puti(hz);
    Console::puts(" MS has passed\n");
    Thread *thread = Thread::CurrentThread();

    /* Resume to the current thread and yield.*/
    resume(thread);
    yield();
  }
}