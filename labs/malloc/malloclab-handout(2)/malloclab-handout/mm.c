/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

void *current_avail = NULL;
int current_avail_size = 0; 

/* My globals */

#define listqty 17

typedef struct island_t {
  void* next_island;
  int size;
  int block1_offset;
} island_t;

typedef struct block_t {
  size_t blocksize; /* i.e HEADER (alloc flag = LSB, prevalloc flag = 2nd LSB) */
  void* prev;
  void* next;
}block_t;
#define ALLOC 0x1
#define PREVALLOC 0x2
#define HSIZE 8 /* can serve as: HEADER, prologue, epilogue block */
#define MINFREEBLOCK sizeof(block_t)+HSIZE

island_t* first_island = NULL; /* always poinst to start of whole heap */
island_t* active_island = NULL; /* always points to the last island that was mmapped*/
void* freeArrPtr = NULL;

/* My helper functions */
inline void splitAndInsert(block_t* block, size_t asize, size_t fragmentSize);
inline void LIFO_insert(block_t* newFreeblock, int newIndex);
inline void findIndex(size_t asize, int* index, int* toobig);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  /*- Initialize the heap by calling mmap()
    - Store the Island list metadata
    - Allocate array of free lists at begininng of heap
    - move current_avail ptr forward(add preceeding padding if required), 
      set (its prevalloc flag, size, alloc flag), decrement current_avail_size
    - Initialize segregated free lists (set all heads to NULL)
    */
  current_avail_size = mem_pagesize();
  current_avail = mem_map(current_avail_size);
  if(current_avail == NULL) {
    current_avail_size = 0;
    fprintf(stderr, "Couldn't initialize heap\n");
    return 1;
  }
  /* store the island metadata */
  island_t* island_header = (island_t*)current_avail;
  first_island = island_header;
  island_header->next_island = NULL;
  island_header->size = current_avail_size;

  int hsize = sizeof(island_t);
  current_avail = (void*)((char*)current_avail + hsize);
  current_avail_size -= hsize;

  /* store start of freelist array and move current_avail forward */
  freeArrPtr = current_avail;
  int list_bytes = listqty * sizeof(void*);
  current_avail = (void*)((char*)current_avail + list_bytes);
  current_avail_size -= list_bytes;

  /* Store first block offset in island header and store it in a global */
  island_header->block1_offset = hsize + list_bytes;
  active_island = island_header; /* will be useful to link the islands */

  /*read the next 8 bytes as ptr then deref that and set size, flags*/
  /*hence make epilogue block and also store info abt predecessor block*/
  *(size_t*)current_avail = ALLOC + PREVALLOC; /* set the prevalloc and alloc flags and set size = 0 */

  /*Initialise the seg freelist to all NULLs*/
  /* **So that the freelists are properly NULL terminated */
  void* bp = freeArrPtr;                // OR More Cleanly:- 
  for (int _ = 0; _ < listqty; _++) {   // void **seg_list = (void**)freeArrPtr; 
    *(void**)bp = NULL;                 // for (int i = 0; i < listqty; i++) {
    bp = (void*)((char*)bp + HSIZE);        //    seg_list[i] = NULL; }
  }                                     

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  // adjusted size is 16 byte aligned block size
  size_t asize = ALIGN(size + HSIZE/*header*/);
  if (asize < 32) asize = 32; // since, if size < 9, asize = 16 (will break when this block is freed)
  void *p;

  /* Get the freelist index */
  int index = -1; int toobig = 0;
  findIndex(asize, &index, &toobig);

  void **seg_list = (void**)freeArrPtr;
  /* Search(first-fit) in the freelist */
  if(!toobig){
    void* freelist = seg_list[index];

    /* if freelist is non empty */
    /* Search in this freelist for suitable freeblock. if cant find go to higher freelist 
       if cant find in whole freelist array allocate from virgin heap space or OS */
    if (freelist != NULL) {
      // traverse the freelist till you find a freeblock that is >= asize (first-fit)
      block_t* currentblock = (block_t*)freelist;

      while(currentblock != NULL) {
        size_t blocksize = currentblock->blocksize & ~0xF;
        if (blocksize >= asize) {
          // unlink this block from the freelist but maintain the freelist
          block_t* prevblock = (block_t*)currentblock->prev;
          block_t* nextblock = (block_t*)currentblock->next;
          if (prevblock != NULL) {
            prevblock->next = (void*)nextblock;
          } else // i.e if currentblock is head of freelist
              seg_list[index] = (void*)nextblock; /*freelist is a local ptr var. Updating that wont update the actual freelist */
          if (nextblock != NULL) {
            nextblock->prev = (void*)prevblock;
          }

          // allocate the block, split & reinsert if applicable, also **update flag in successor blocks**
          currentblock->blocksize |= ALLOC;
          size_t fragmentsize = blocksize - asize;
          if (fragmentsize >= MINFREEBLOCK) {
            splitAndInsert(currentblock, asize, fragmentsize); // it auto updates the flags in successor blocks
          } else {
            // set prevalloc flag to 1 in the successor block after whole block
            *(size_t*)((char*)currentblock + blocksize) |= PREVALLOC;
          }

          p = (void*)&currentblock->prev; // we want payload to overwrite the prev,next,footer
          return p;
        }
        currentblock = (block_t*)(currentblock->next);
      }
    }

    /* indexed freelist is empty or no suitable block in it */
    /* First check higher freelists(first-fit).
        If none contain suitable block-> allocate from current_avail. 
        If current_avail_size is not enough-> allocate new page */
    int counter = index;
    for (int i = index + 1; i < listqty; i++) {
      freelist = seg_list[i];
      if (freelist == NULL) {
        counter++;
        continue;
      }
      /* if a higher freelist is not empty 
          No need to traverse it as any block in this freelist > 'asize' */
      block_t* block = (block_t*) freelist; 
      size_t blocksize = block->blocksize & ~0xF;
      // **Unlink the block from the freelist
      seg_list[i] = block->next;
      if(block->next != NULL) {
        ((block_t*)block->next)->prev = NULL;
        block->next = NULL; 
      }
      block->prev = NULL; // although not needed due to how we insert a block in a freelist, still good hygiene

    /* allocate the block; break this block up, put footer at end of new free block 
       and insert into suitable freelist - (if applicable) */
      block->blocksize |= ALLOC; // Set the alloc bit 
      size_t fragmentSize = blocksize - asize; // The fragmentSize is always 16byte aligned(as blocksize and asize are always 16byte aligned)
      if (fragmentSize >= MINFREEBLOCK) { 
        splitAndInsert(block, asize, fragmentSize);
      } else {
        // set prevalloc flag to 1 in the successor block after whole block
        *(size_t*)((char*)block + blocksize) |= PREVALLOC;
      }

      p = (void*)&block->prev; /* i.e p = (void*)((char*)block + 8); */
      return p;
    }

  /* Couldnt find any suitable block in freelist array-> Allocate from current_avail 
     current_avail doesnt hv enough space-> map new pages and migrate current_avail there */

    if (counter == listqty - 1) { // if all freelists searched but couldnt fine suitable block
      if (current_avail_size >= asize + HSIZE) { // if the block & epilogue can fit within virgin heap space
        
        // Allocate the block and shrink virgin heap space
        int prevalloc = (*(size_t*)current_avail) & PREVALLOC; // get the alloc status of the predecessor block
        block_t* freshblock = (block_t*)current_avail;
        freshblock->blocksize = asize;
        freshblock->blocksize |= (ALLOC + prevalloc);
        
        // Put the epilogue
        current_avail = (void*)((char*)current_avail + asize);
        current_avail_size -= asize;
        *(size_t*)current_avail = ALLOC + PREVALLOC;

        p = (void*)&freshblock->prev;
        return p;
      } 
      else { /* get new page from OS, make walls in it, free virgin space in prev island(if applicable), 
        link with prev island then allocate from here */

        // for new islands we need a prologue(or 8byte padding) since we want blocks to start at an 8byte offset addr
        int newIslandsize = PAGE_ALIGN(sizeof(island_t) + HSIZE/*prologue*/ + asize + HSIZE/*epilogue*/);
        void* newIslandptr = mem_map(newIslandsize);

        island_t* newIslandHeader = (island_t*)newIslandptr;
        newIslandHeader->size = newIslandsize;
        newIslandHeader->next_island = NULL;
        active_island->next_island = newIslandptr; // link this island to previously active one
        active_island = newIslandHeader; // update the active_island to point to this current one

        // make prologue(padding) block- remember to set the prevalloc flag of the next block
        newIslandptr = (void*)((char*)newIslandptr + sizeof(island_t)); 
        *(size_t*)newIslandptr = ALLOC + PREVALLOC; /* prevalloc flag of this padding block should likely never be checked 
                                                        bt if it does this is some safety atleast */
        // allocate the asize block
        newIslandptr = (void*)((char*)newIslandptr + HSIZE); // mv it to after padding
        block_t* freshblock = (block_t*)newIslandptr;
        freshblock->blocksize = (asize | (ALLOC + PREVALLOC)); // remember to set prevalloc flag of next 8byte block
        newIslandptr = (void*)((char*)newIslandptr + asize); // mv it to after the allocated block
        int usedbytes = sizeof(island_t)/*island header*/ + HSIZE/*padding*/ + asize; // store for use by current_avail_size
        newIslandHeader->block1_offset = usedbytes - asize;

        /* free virgin space in prev active island (if bigger than min freeblock size),
            make epilogue after migrating current_avail here */
        current_avail_size -= HSIZE;/* size without epilogue of tht island */
        if (current_avail_size/*in prev active island*/ >= MINFREEBLOCK) { 
          int prevalloc = (*(size_t*)current_avail) & PREVALLOC; // store prevalloc flag that was stored in epilogue

          // free the block 
          int freeblocksize = (current_avail_size | PREVALLOC ) & ~ALLOC;
          block_t* newFreeblock = (block_t*)current_avail;
          newFreeblock->blocksize = freeblocksize; 
          // put footer at the end
          current_avail = (void*)((char*)current_avail + current_avail_size - HSIZE/*footer size*/);
          *(size_t*)current_avail = freeblocksize;

          // insert into suitable freelist (LIFO)
          int newIndex = 0;
          findIndex(current_avail_size, &newIndex, NULL);
          LIFO_insert(newFreeblock, newIndex);

          /* re-make the epilogue in prev active island since we overwrote it when we made freeblock from the virgin space */
          current_avail = (void*)((char*)current_avail + HSIZE); // mv it to after the last freeblock in this island
          // write epilogue 
          *(size_t*)current_avail =  ALLOC; // **no PREVALLOC since predecessor block is a freeblock
        }

        // migrate current_avail to new island 
        current_avail = newIslandptr;
        current_avail_size = newIslandsize - usedbytes;
        // make epilogue
        *(size_t*)current_avail = ALLOC + PREVALLOC; 
        // return ptr to user
        p = (void*)&freshblock->prev;
        return p;
      }
    }
  } 
  /* If user asked for a big size which can never be in a freelist */
  else {

  }
}

inline void splitAndInsert(block_t* block, size_t asize, size_t fragmentSize)
{
  // set newsize since we will be splitting this 
  block->blocksize = (asize & ~0xF) | (block->blocksize & 0xF);

  block_t* newFreeblock = (block_t*)((char*)block + asize);
  newFreeblock->blocksize = fragmentSize;
  newFreeblock->blocksize |= PREVALLOC; // set prevalloc flag(since this block is a fragment of a larger free block)
  newFreeblock->blocksize &= ~ALLOC; //clear ALLOC flag. 

  // put footer at end
  size_t* fp = (size_t*)((char*)newFreeblock + fragmentSize - HSIZE/*footer size*/);
  *fp = newFreeblock->blocksize; // contains size and flags

  // insert into a freelist (LIFO)
  int newIndex = 0;
  findIndex(fragmentSize, &newIndex, NULL);
  LIFO_insert(newFreeblock, newIndex);

  // set prevalloc flag to 0 in the successor block after newFreeblock
  *(size_t*)((char*)newFreeblock + fragmentSize) &= ~PREVALLOC;
}

inline void LIFO_insert(block_t* newFreeblock, int newIndex)
{
  void** seg_list = (void**)freeArrPtr;

  void* ourfreelist = seg_list[newIndex];
  if (ourfreelist != NULL) {
    block_t* frist_block = (block_t*)ourfreelist;
    frist_block->prev = (void*)newFreeblock;
  }
  newFreeblock->next = seg_list[newIndex];
  newFreeblock->prev = NULL;
  seg_list[newIndex] = (void*)newFreeblock;
}

inline void findIndex(size_t asize, int* index, int* toobig) {
  if (asize > (1<<20)) {
    if(asize <= 4*(1<<20))
      *index = 16; 
    else {
      if (toobig != NULL)
        *toobig = 1;
    }
  } else {
    *index = 0;
    while (asize > 32) {
      asize >>= 1;
      (*index)++;
    }
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  return 1;
}
