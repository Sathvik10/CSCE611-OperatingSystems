/*
 File: ContFramePool.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/*
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates
 *single* frames at a time. Because it does allocate one frame at a time,
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.

 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.

 This can be done in many ways, ranging from extensions to bitmaps to
 free-lists of frames etc.

 IMPLEMENTATION:

 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame,
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool.
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.

 NOTE: If we use this scheme to allocate only single frames, then all
 frames are marked as either FREE or HEAD-OF-SEQUENCE.

 NOTE: In SimpleFramePool we needed only one bit to store the state of
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work,
 revisit the implementation and change it to using two bits. You will get
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.

 DETAILED IMPLEMENTATION:

 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:

 Constructor: Initialize all frames to FREE, except for any frames that you
 need for the management of the frame pool, if any.

 get_frames(_n_frames): Traverse the "bitmap" of states and look for a
 sequence of at least _n_frames entries that are FREE. If you find one,
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.

 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.

 needed_info_frames(_n_frames): This depends on how many bits you need
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.

 A WORD ABOUT RELEASE_FRAMES():

 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e.,
 not associated with a particular frame pool.

 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete

 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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

ContFramePool *ContFramePool::pools;
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    assert(_n_frames <= FRAME_SIZE * 8);

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    nextPool = nullptr;

    // If _info_frame_no is zero then we keep management info in the first
    // frame, else we use the provided frame to keep management info
    if (info_frame_no == 0)
    {
        bitmap = (unsigned char *)(base_frame_no * FRAME_SIZE);
    }
    else
    {
        bitmap = (unsigned char *)(info_frame_no * FRAME_SIZE);
    }

    // Everything ok. Proceed to mark all frame as free.
    for (int fno = 0; fno < _n_frames; fno++)
    {
        set_state(fno, FrameState::Free);
    }

    // Mark the first frame as being used if it is being used
    if (_info_frame_no == 0)
    {
        set_state(0, FrameState::Used);
        nFreeFrames--;
    }

    // Maintaining the list of pools
    if (ContFramePool::pools == nullptr)
    {
        pools = this;
    }
    else
    {
        ContFramePool *curPool = pools;
        while (curPool->nextPool != nullptr)
        {
            curPool = curPool->nextPool;
        }
        curPool->nextPool = this;
    }

    Console::puts("Frame Pool initialized\n");
}

ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no)
{
    unsigned int frame_idx = _frame_no / 4;
    unsigned int frame_col = _frame_no % 4;
    unsigned char mask = 11 << (frame_col * 2);
    unsigned char state = (bitmap[frame_idx] & mask) >> (frame_col * 2);
    unsigned int state_int = ((unsigned int)state);
    if (state == 0)
    {
        return FrameState::Free;
    }
    else if (state == 1)
    {
        return FrameState::Used;
    }
    else if (state == 3)
    {
        return FrameState::HoS;
    }
    else
    {
        return FrameState::Head;
    }
}

unsigned char ContFramePool::get_state_value(ContFramePool::FrameState _state)
{
    switch (_state)
    {
    case FrameState::Free:
        return (unsigned int)0;
    case FrameState::Used:
        return (unsigned int)1;
    case FrameState::HoS:
        return (unsigned int)3;
    case FrameState::Head:
        return (unsigned int)2;
    default:
        return 0;
    }
}

void ContFramePool::set_state(unsigned long _frame_no, ContFramePool::FrameState _state)
{
    unsigned int frame_idx = _frame_no / 4;
    unsigned short frame_col = _frame_no % 4;
    unsigned char state = bitmap[frame_idx];
    unsigned char mask = 0xFF;
    mask &= ~(1 << (frame_col * 2));
    mask &= ~(1 << (frame_col * 2 + 1));

    state &= mask;
    mask = get_state_value(_state) << (frame_col * 2);
    state |= mask;
    bitmap[frame_idx] = state;
}

bool ContFramePool::is_valid_frame(unsigned long _frame_no)
{
    unsigned long frame_no = _frame_no - base_frame_no;
    if (frame_no < 0 || frame_no >= nframes)
        return false;
    return true;
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    if (_n_frames <= 0)
    {
        return 0;
    }
    if (_n_frames > nFreeFrames)
    {
        Console::puts("Unable to allocate frames\n");
        return 0;
    }
    unsigned long base_no = 0;
    bool found = false;
    while (base_no < nframes)
    {
        if (get_state(base_no) == FrameState::Free && is_valid_frame(base_frame_no + base_no + _n_frames - 1))
        {
            unsigned int count = 1;
            unsigned long next_frame = base_no + 1;
            while (get_state(next_frame) == FrameState::Free && count < _n_frames)
            {
                count++;
                next_frame++;
            }
            if (count == _n_frames)
            {
                found = true;
                break;
            }
            base_no = next_frame;
        }
        base_no++;
    }
    if (found)
    {
        set_state(base_no, FrameState::Head);
        nFreeFrames--;
        for (unsigned int i = 1; i < _n_frames; i++)
        {
            set_state(base_no + i, FrameState::Used);
            nFreeFrames--;
        }
        return base_no + base_frame_no;
    }

    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    if (!is_valid_frame(_base_frame_no))
        return;
    unsigned long frame_no = _base_frame_no - base_frame_no;
    for (unsigned int i = 0; i < _n_frames; i++)
    {
        if (is_valid_frame(frame_no + i))
            set_state(frame_no + i, FrameState::HoS);
    }
}

void ContFramePool::free_frames(unsigned long _frame_no)
{
    unsigned long frame_no = _frame_no - base_frame_no;
    if (get_state(frame_no) == FrameState::Head)
    {
        do
        {
            set_state(frame_no, FrameState::Free);
            frame_no++;
            nFreeFrames++;
        } while (get_state(frame_no) == FrameState::Used);
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    ContFramePool *cur_pool = ContFramePool::pools;
    while (cur_pool != nullptr)
    {
        if (cur_pool->is_valid_frame(_first_frame_no))
        {
            cur_pool->free_frames(_first_frame_no);
            cur_pool = nullptr;
            break;
        }
        cur_pool = cur_pool->nextPool;
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    unsigned long total_bytes = ((_n_frames * 2) / 8);
    unsigned long total_frames = total_bytes / FRAME_SIZE;
    if (total_bytes % FRAME_SIZE != 0)
        total_frames++;
    return total_frames;
}
