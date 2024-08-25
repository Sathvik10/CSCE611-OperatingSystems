/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
#define FILE_SIZE 512
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

// Initiate all the variables.
// Read the file block into the cache.
File::File(FileSystem *_fs, int _id)
{
    Console::puts("Opening file.\n");
    fs = _fs;
    file_id = _id;
    current_position = 0;
    inode = fs->LookupFile(_id);
    fs->read_block_from_disk(inode->block_no, block_cache);
}

// Write back the block to the disk.
File::~File()
{
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */
    fs->write_block_to_disk(inode->block_no, block_cache);
    fs->write_inode_to_disk();
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

// Read n characters from the cached block.
int File::Read(unsigned int _n, char *_buf)
{
    Console::puts("reading from file\n");
    unsigned long index = 0;

    while (!EoF() && index < _n)
    {
        _buf[index] = block_cache[current_position];
        current_position++;
        index++;
    }
    return index;
}

// write n characters to the cached block.
int File::Write(unsigned int _n, const char *_buf)
{
    Console::puts("writing to file\n");
    unsigned long new_position = current_position + _n;

    if (new_position > inode->size)
    {
        inode->size = new_position;
    }

    if (inode->size > FILE_SIZE)
    {
        inode->size = FILE_SIZE;
    }

    unsigned long index = 0;
    while (!EoF() && index < _n)
    {
        block_cache[current_position] = _buf[index];
        current_position++;
        index++;
    }
    return index;
}

// Reset the position from the current position.
void File::Reset()
{
    Console::puts("resetting file\n");
    current_position = 0;
}

// Check if the current position reached the end of file.
bool File::EoF()
{
    Console::puts("checking for EoF\n");
    if (current_position >= inode->size)
    {
        return true;
    }
    else
    {
        return false;
    }
}
