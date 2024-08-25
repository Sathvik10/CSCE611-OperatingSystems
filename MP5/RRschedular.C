#include "console.H"
#include "scheduler.H"
#include "simple_timer.H"
#include "thread.H"

void idealThreadFunc()
{
    Console::puts("Ideal Thread Starting\n");
    for (int i = 0; i < 1000; i++)
    {
        for (int j = 0; j < 100000; j++)
        {
        }
    }
}

class RRSchedular : public Scheduler, public SimpleTimer
{
private:
public:
    RRSchedular(int _hz) : SimpleTimer(_hz), Scheduler()
    {
    }

    virtual void yield() override
    {
        ticks = 0;

        Machine::outportb(0X20, 0X20);
        if (Machine::interrupts_enabled())
        {
            Machine::disable_interrupts();
        }
        Thread *next_thread;
        if (head == nullptr)
        {
            char *idealStack = new char[1024];
            Thread *idealThread = new Thread(idealThreadFunc, idealStack, 1024);
            Console::puts("Yeilding thread to ideal thread\n");
            next_thread = idealThread;
        }
        else
        {
            Node *next = head;
            head = head->next;

            next_thread = next->thread;
            delete next;

            Console::puts("RR Scheduler Yeilding Thread To [");
            Console::puti(next_thread->ThreadId());
            Console::puts("]\n");
        }

        if (!Machine::interrupts_enabled())
        {
            Machine::enable_interrupts();
        }

        Thread::dispatch_to(next_thread);
    }

    virtual void handle_interrupt(REGS *_r) override
    {
        ticks++;

        /* Whenever a second is over, we update counter accordingly. */
        if (ticks >= hz)
        {
            seconds++;
            ticks = 0;
            Console::puti(hz);
            Console::puts(" MS has passed\n");
            Thread *thread = Thread::CurrentThread();
            resume(thread);
            yield();
        }
    }
};