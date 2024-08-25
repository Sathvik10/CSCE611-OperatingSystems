/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

#define INODE_BLOCK 0
#define FILES_BLOCK 1
#define BLOCK_SIZE 512
#define NO_META_BLOCKS 2

FileSystem::FileSystem()
{
    Console::puts("In file system constructor.\n");

    inodes = new Inode[MAX_INODES];

    free_blocks = new unsigned char[BLOCK_SIZE];

    no_files = 0;

    no_nodes = 0;

    disk = nullptr;

    size = 0;
}

FileSystem::~FileSystem()
{
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    write_inode_to_disk();
    write_free_list_to_disk();

    delete[] inodes;
    delete[] free_blocks;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

// Copy the reference of disk into your variable
// Read the first block into inode cache and second block into free list cache
// Check whether the first two blocks are marked as used in the free list
bool FileSystem::Mount(SimpleDisk *_disk)
{
    Console::puts("mounting file system from disk\n");

    disk = _disk;

    disk->read(INODE_BLOCK, (unsigned char *)inodes);

    disk->read(FILES_BLOCK, free_blocks);

    if (free_blocks[INODE_BLOCK] == 1 && free_blocks[FILES_BLOCK] == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool FileSystem::Format(SimpleDisk *_disk, unsigned int _size)
{ // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    unsigned int i = 0;
    unsigned char buffer[BLOCK_SIZE];
    Inode *buf_inodes = new Inode[MAX_INODES];

    // Initialize inode block to -1
    for (i = 0; i < MAX_INODES; i++)
    {
        buf_inodes[i].id = -1;
    }

    _disk->write(INODE_BLOCK, (unsigned char *)buf_inodes);

    // Initialize File List to 0
    for (i = 0; i < BLOCK_SIZE; i++)
    {
        buffer[i] = 0;
    }
    // Mark the first two blocks as used
    buffer[INODE_BLOCK] = 1;
    buffer[FILES_BLOCK] = 1;

    // Write the updated file block to disk
    _disk->write(FILES_BLOCK, buffer);

    delete[] buf_inodes;
    delete[] buffer;

    return true;
}

Inode *FileSystem::LookupFile(int _file_id)
{
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you go through the inode list to find the file. */

    for (int i = 0; i < no_nodes; i++)
    {
        if (inodes[i].id == _file_id)
        {
            return &inodes[i];
        }
    }
    return nullptr;
}

void FileSystem::write_inode_to_disk()
{
    disk->write(INODE_BLOCK, (unsigned char *)inodes);
}

void FileSystem::write_free_list_to_disk()
{
    disk->write(FILES_BLOCK, (unsigned char *)free_blocks);
}

void FileSystem::write_block_to_disk(unsigned long block_no, unsigned char *buf)
{
    disk->write(NO_META_BLOCKS + block_no, buf);
}

void FileSystem::read_block_from_disk(unsigned long block_no, unsigned char *buf)
{
    disk->read(NO_META_BLOCKS + block_no, buf);
}

unsigned long FileSystem::find_free_block()
{
    for (unsigned long i = 0; i < no_files; i++)
    {
        if (free_blocks[i] == 0)
        {
            return i;
        }
    }

    return -1;
}

unsigned long FileSystem::find_free_inode()
{
    for (unsigned long i = 0; i < no_nodes; i++)
    {
        if (inodes[i].id == -1)
        {
            return i;
        }
    }
    return -1;
}

bool FileSystem::CreateFile(int _file_id)
{
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    if (LookupFile(_file_id) != nullptr)
    {
        return false;
    }

    unsigned long free_block_no = find_free_block();

    if (free_block_no == -1)
    {
        if (no_files == BLOCK_SIZE - 2)
        {
            Console::puts("CreateFile: free blocks not available \n");
            return false;
        }
        free_block_no = no_files;
        no_files++;
    }

    // Can improve
    unsigned long free_inode_no = find_free_inode();
    if (free_inode_no == -1)
    {
        if (no_nodes == MAX_INODES)
        {
            Console::puts("CreateFile: iNodes not available \n");
            return false;
        }
        free_inode_no = no_nodes;
        no_nodes++;
    }

    free_blocks[free_block_no] = 1;
    inodes[free_inode_no].size = 0;
    inodes[free_inode_no].id = _file_id;
    inodes[free_inode_no].block_no = free_block_no;

    disk->write(INODE_BLOCK, (unsigned char *)inodes);
    disk->write(FILES_BLOCK, (unsigned char *)free_blocks);

    Console::puts("CreateFile: File is created.");

    return true;
}

bool FileSystem::DeleteFile(int _file_id)
{
    Console::puts("deleting file with id:");
    Console::puti(_file_id);
    Console::puts("\n");

    Inode *file_inode = LookupFile(_file_id);
    if (file_inode == nullptr)
    {
        Console::puts("DeleteFile: File not found.");
        return false;
    }

    free_blocks[file_inode->block_no] = 0;
    file_inode->id = -1;

    disk->write(INODE_BLOCK, (unsigned char *)inodes);
    disk->write(FILES_BLOCK, (unsigned char *)free_blocks);

    Console::puts("DeleteFile: File is deleted.");
    return true;
}
