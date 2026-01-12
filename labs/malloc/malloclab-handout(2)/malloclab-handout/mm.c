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

/* rounds down to the nearest multiple of mem_pagesize() */
#define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

int ptr_is_mapped(void *p, size_t len) {
  void *s = ADDRESS_PAGE_START(p);
  return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));
}

/* My globals */

#define listqty 17
#define FIRSTISLAND 0x49534c414e445254 // "ISLANDRT"
#define NORMISLAND 0x49534c414e444e4d  // "ISLANDNM"

typedef struct island_t { // 32 bytes
  void* prev_island;
  void* next_island;
  size_t size;
  size_t magic_number; /* Will distingush first island from the rest */
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
island_t* spare_island = NULL;
void* freeArrPtr = NULL;

/* My helper functions */
inline void* unlinkAllocReinsert(block_t* chosenblock, size_t blocksize, size_t asize, int index);
inline void freelistUnlink(block_t* chosenblock, int index);
inline void splitAndInsert(block_t* block, size_t asize, size_t fragmentSize);
inline void LIFO_insert(block_t* newFreeblock, int newIndex);
inline void findIndex(size_t asize, int* index, int* toobig);
int my_check();
int check_freelists();

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
  current_avail_size = mem_pagesize()*4;
  current_avail = mem_map(current_avail_size);
  if(current_avail == NULL) {
    current_avail_size = 0;
    fprintf(stderr, "Couldn't initialize heap\n");
    return 1;
  }
  /* store start of freelist array and move current_avail forward */
  freeArrPtr = current_avail;
  int list_bytes = listqty * HSIZE;
  current_avail = (void*)((char*)current_avail + list_bytes); // mv after freeArrPtr
  current_avail_size -= list_bytes;

  /* store the island metadata */
  island_t* island_header = (island_t*)current_avail;
  first_island = island_header;
  island_header->next_island = NULL;
  island_header->prev_island = NULL;
  island_header->size = current_avail_size + list_bytes;
  /* **In free() cmp prologue with FIRSTISLAND to check if we are in first_island */
  /*---------------------------------------*/
  island_header->magic_number = FIRSTISLAND;
  /*---------------------------------------*/

  int hsize = sizeof(island_t);
  active_island = island_header; /* will be useful to link the islands */

  // mv to after the island header
  current_avail = (void*)((char*)current_avail + hsize);
  current_avail_size -= hsize;

  /*read the next 8 bytes as ptr then deref that and set size, flags*/
  /*hence make epilogue block and also store info abt predecessor block*/
  *(size_t*)current_avail = ALLOC + PREVALLOC; /* set the prevalloc and alloc flags and set size = 0 */

  /*Initialise the seg freelist to all NULLs*/
  /* **So that the freelists are properly NULL terminated */
  void* bp = freeArrPtr;                // OR More Cleanly:- 
  for (int _ = 0; _ < listqty; _++) {   // void **seg_list = (void**)freeArrPtr; 
    *(void**)bp = NULL;                 // for (int i = 0; i < listqty; i++) {
    bp = (void*)((char*)bp + HSIZE);    //    seg_list[i] = NULL; 
  }                                     // }

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
      block_t* currentblock = (block_t*)freelist;
      size_t blocksize = currentblock->blocksize & ~0xF;
      
      if (index < listqty - 1) {
        // else traverse the freelist till you find a freeblock that is >= asize (first-fit)
        while(currentblock != NULL) {
          blocksize = currentblock->blocksize & ~0xF;
          if (blocksize >= asize) {
            p = unlinkAllocReinsert(currentblock, blocksize, asize, index); 
            return p;
          }
          currentblock = (block_t*)(currentblock->next);
        }
      }
      /* RARE */
      // if size req is in (1MB, 4MB-32], use best-fit in the last freelist 
      else {
        block_t* chosenblock = NULL;
        size_t leastWaste = 4*(1<<20)-32 - asize; // initialise to most wasted space there can be
        size_t fragmentSize;

        // Choose the best fit block
        while(currentblock != NULL) {
          blocksize = currentblock->blocksize & ~0xF;

          if (blocksize < asize) { 
            /* Empty on Purpose */ 
          }
          else if (blocksize == asize) {
            leastWaste = 0;
            chosenblock = currentblock;
            break;
          }
          else {
            fragmentSize = blocksize - asize;
            if (fragmentSize < leastWaste) {
              leastWaste = fragmentSize;
              chosenblock = currentblock;
            }
          }
          currentblock = (block_t*)(currentblock->next);
        }

        // allocate the chosen block + split & reinsert(if applicable)
        if (chosenblock != NULL) {
          p = unlinkAllocReinsert(chosenblock, chosenblock->blocksize & ~0xF, asize, index);
          return p;
        }
      }
    }

    /* indexed freelist is empty or no suitable block in it */
    /* First check higher freelists(first-fit).
        If none contain suitable block-> allocate from current_avail. 
        If current_avail_size is not enough-> allocate new page */
    int counter = index; // if index = 16 it will never enter this loop
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

      p = (void*)((char*)block + HSIZE); /* i.e p = (void*)((char*)block + 8); */
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

        p = (void*)((char*)freshblock + HSIZE);
        return p;
      } 
      else { /* get new page from OS, make walls in it, free virgin space in prev island(if applicable), 
        link with prev island then allocate from here */

        size_t newIslandsize = PAGE_ALIGN(sizeof(island_t)+ 2*HSIZE/*prologue+epilogue*/ + asize);
        void* newIslandptr = mem_map(newIslandsize);

        island_t* newIslandHeader = (island_t*)newIslandptr;
        newIslandHeader->size = newIslandsize;
        newIslandHeader->next_island = NULL;
        newIslandHeader->magic_number = NORMISLAND;
        newIslandHeader->prev_island = active_island;
        active_island->next_island = newIslandptr; // link this island to previously active one
        active_island = newIslandHeader; // update the active_island to point to this current one

        /* Prologue (will be used in free() to check if whole island is free & also gives 8byte offset) */
        newIslandptr = (void*)((char*)newIslandptr + sizeof(island_t));
        *(size_t*)newIslandptr = ALLOC + PREVALLOC;

        // allocate the asize block
        newIslandptr = (void*)((char*)newIslandptr + HSIZE/*prologue*/);
        block_t* freshblock = (block_t*)newIslandptr;
        freshblock->blocksize = asize | (ALLOC + PREVALLOC); // remember to set prevalloc flag of the epilogue
        newIslandptr = (void*)((char*)newIslandptr + asize); // mv it to after the allocated block
        int usedbytes = sizeof(island_t)/*island header*/ + HSIZE/*prologue*/ + asize; // store for use by current_avail_size

        /* free virgin space in prev active island (if bigger than min freeblock size),
            make epilogue after migrating current_avail here */
        current_avail_size -= HSIZE;/* size without epilogue of tht island */
        if (current_avail_size/*in prev active island*/ >= MINFREEBLOCK) { 
          int prevalloc = (*(size_t*)current_avail) & PREVALLOC; // store prevalloc flag that was stored in epilogue

          // free the block 
          size_t freeblocksize = (current_avail_size | prevalloc) & ~ALLOC;
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
        // make epilogue in newisland
        *(size_t*)current_avail = ALLOC + PREVALLOC; 
        // return ptr to user
        p = (void*)((char*)freshblock + HSIZE);
        return p;
      }
    }
  } 
  /* If user asked for a big size which can never be in a freelist */
  else {
    // dont make an island just give whole chunk returned by OS to user. 
    // When user calls free() on it(check if blocksize > 4MB in free()) just return it to OS
    if (asize <= ~0) { // ~0 = (2^32 - 1) as asize is of type size_t, ~0 will be treated as size_t
      size_t chonksize = PAGE_ALIGN(asize + HSIZE/*padding for 8byte offset alignment*/);
      void* startaddr = mem_map(chonksize);
      if (startaddr == NULL) {
        fprintf(stderr, "OS could not allocate %zu bytes\nReturning NULL\n", asize);
        return NULL;
      }
      // make prologue or padding 
      *(size_t*)startaddr = chonksize;
      startaddr = (void*)((char*)startaddr + HSIZE);

      block_t* block = (block_t*)startaddr;
      block->blocksize = asize | (ALLOC + PREVALLOC); // blockheader will serve as left wall

      p = (void*)&block->prev;
      return p;
    }
    else {
      fprintf(stderr, "Requested size too large. Aborting...\nReturned NULL\n");
      return NULL;
    }
  }
}

inline void* unlinkAllocReinsert(block_t* chosenblock, size_t blocksize, size_t asize, int index)
{
  // unlink this block from the freelist but maintain the freelist
  freelistUnlink(chosenblock, index);

  // allocate the block, split & reinsert if applicable, also **update flag in successor blocks**
  chosenblock->blocksize |= ALLOC;
  size_t fragmentsize = blocksize - asize;
  if (fragmentsize >= MINFREEBLOCK) {
    splitAndInsert(chosenblock, asize, fragmentsize); // it auto updates the flags in successor blocks
  } else {
    // set prevalloc flag to 1 in the successor block after whole block
    *(size_t*)((char*)chosenblock + blocksize) |= PREVALLOC;
  }

  return (void*)((char*)chosenblock + HSIZE);// we want payload to overwrite the prev,next,footer
}

inline void freelistUnlink(block_t* chosenblock, int index) 
{
  void** seg_list = (void**)freeArrPtr;

  block_t* prevblock = (block_t*)chosenblock->prev;
  block_t* nextblock = (block_t*)chosenblock->next;
  if (prevblock != NULL) {
    prevblock->next = (void*)chosenblock->next;
  } else // i.e if chosenblock is head of freelist
      seg_list[index] = (void*)chosenblock->next;
  if (nextblock != NULL) {
    nextblock->prev = (void*)chosenblock->prev;
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
    if(asize <= 4*(1<<20)-32/*island overhead*/)
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
  /*Freeing will basically reinsert a block in a freelist 
    - But before doing that you need to coalesce if aplicable by checking successor block and 
      predecessor block(by looking at the flag first then checking its footer)'s alloc status
    
    - When coalescing do not forget to remove the neighbors from the free list before merging
      If the previous block is free, it is currently sitting in some linked list in seg_list. 
      If you merge with it and change its size/pointers without removing it from that list, u 
      corrupt your data structure.

    - After coalescing, remake the block header & footer then re-insert(LIFO) into a freelist
      also update prevalloc flag of block after coalesced block 

    - Also if blocksize > 4MB, return it to the OS(mem_unmap())

    - mem_unmap() can also be used for memory optimization to reduce total size of pages used 
      by your allocator at a point(increases utilization score).
  */

  if (ptr == NULL) return;
  
  block_t* block = (block_t*)((char*)ptr - HSIZE); // ptr points to start of payload 
  size_t blocksize = block->blocksize & ~0xF;
  if (blocksize > 4*(1<<20)-32) { // standalone large block
    mem_unmap((char*)ptr - 2*HSIZE, *(size_t*)((char*)ptr - 2*HSIZE));
    return;
  }

  int prevalloc = block->blocksize & PREVALLOC;
  int nextalloc = (*(size_t*)((char*)block + blocksize)) & ALLOC;
  size_t pred_blocksize = 0; size_t succ_blocksize = 0;
  if (!nextalloc) {
    succ_blocksize = *(size_t*)((char*)block + blocksize) & ~0xF;
    int index = 0;
    findIndex(succ_blocksize, &index, NULL);
    freelistUnlink((block_t*)((char*)block + blocksize), index);
  }
  if (!prevalloc) { // then there must be footer
    pred_blocksize = *(size_t*)((char*)block - HSIZE) & ~0xF;
    int index = 0;
    findIndex(pred_blocksize, &index, NULL);
    freelistUnlink((block_t*)((char*)block - pred_blocksize), index);
  }

  size_t coalesce_blocksize = blocksize + pred_blocksize + succ_blocksize;
  block_t* coalesce_block = (block_t*)((char*)block - pred_blocksize);
  coalesce_block->blocksize = coalesce_blocksize | (coalesce_block->blocksize & PREVALLOC); // Preserves PREVALLOC, forces ALLOC to 0
  *(size_t*)((char*)coalesce_block + coalesce_blocksize) &= ~PREVALLOC; // clear prevalloc flag of coalesce block's successor block
  *(size_t*)((char*)coalesce_block + coalesce_blocksize - HSIZE) = coalesce_block->blocksize; // make footer
  /*
  // now check if whole island was freed (munmap() optimization)
  size_t potprolog = *(size_t*)((char*)coalesce_block - HSIZE);
  size_t potepilog = *(size_t*)((char*)coalesce_block + coalesce_blocksize);
  int whole = 0;
  if (!((potprolog & ~0xF)+(potepilog & ~0xF)))
    whole = 1;
    
  if (whole) {
    island_t* isl_header = (island_t*)((char*)coalesce_block - HSIZE - sizeof(island_t));
    // unlink from islands linkedlist and return to os if islandsize < spareislandsize 
    // if islandsize > spareislandsize or spareisland == NULL, cache this island.

    island_t* next_isl = (island_t*)isl_header->next_island; 
    island_t* prev_isl = (island_t*)isl_header->prev_island; 

    if (isl_header != active_island) { 
      // Since we know it's not first_island, prev_isl is guaranteed NOT NULL
      if (prev_isl != NULL)
        prev_isl->next_island = isl_header->next_island;
      
      if (next_isl != NULL) {
        next_isl->prev_island = isl_header->prev_island;
      }
      mem_unmap((void*)isl_header, isl_header->size);
      return;
    }
  }
  */

  // else insert into a freelist
  int newIndex = 0;
  findIndex(coalesce_blocksize, &newIndex, NULL);
  LIFO_insert(coalesce_block, newIndex);
  // update the prevalloc flag of successor block
  size_t* succblockHeader = (size_t*)((char*)coalesce_block + coalesce_blocksize);
  *succblockHeader = (*succblockHeader) & ~PREVALLOC; 
}

/* My heap_checker so that dont hv to write mm_can_free() yet */
int my_check() // check the whole heap at any point in time
{
  // start from first island until NULL 
  /* check blocks in every island i.e if they have proper structure
      - i.e no physical overlap in allocated blocks
      - freeblock block structure is correct
  */


}

/* This will check the logical integrity of the freelists */
int check_freelists()
{
  /* Check all the freelists to chekc if there was logical overlap
     or if there was improper accounting(no. of freebocks counter in
     physical scan dont match with actual number of freeblocks)
     Also check for cycles.
     Also check if a freeblock really belongs in a particular freelist
  */


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
