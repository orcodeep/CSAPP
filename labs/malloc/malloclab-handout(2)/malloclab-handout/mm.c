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
  size_t blocksize;
  void* prev;
  void* next;
}block_t;

island_t* first_island = NULL; /* always poinst to start of whole heap */
island_t* active_island = NULL; /* always points to the last island that was mmapped*/
void* freeArrPtr = NULL;

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
  *(size_t*)current_avail = 0x3; /* set the prevalloc and alloc flags and set size = 0 */

  /*Initialise the seg freelist to all NULLs*/
  void* bp = freeArrPtr;                // OR More Cleanly:- 
  for (int _ = 0; _ < listqty; _++) {   // void **seg_list = (void**)freeArrPtr; 
    *(void**)bp = NULL;                 // for (int i = 0; i < listqty; i++) {
    bp = (void*)((char*)bp + 8);        //    seg_list[i] = NULL; }
  }                                     

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
void *mm_malloc(size_t size)
{
  /* 
  if (current_avail_size < newsize) {
    current_avail_size = PAGE_ALIGN(newsize);
    current_avail = mem_map(current_avail_size);
    if (current_avail == NULL)
      return NULL;
  } 

  p = current_avail;
  current_avail += newsize;
  current_avail_size -= newsize;
  return p;
  */

  /*First we will search in its index
    Go higher if not found in its index
    Allocate from current_avail if couldn't find in whole freelist
    If current_avail_size too small make new island */

  /*adjusted size is 16 byte aligned block size */
  size_t asize = ALIGN(size + sizeof(size_t));
  void *p;

  /* Get the freelist index */
  int index = -1; int toobig = 0;
  if (asize > (1<<20)) {
    if(asize < (4*(1<<20) + 1))
      index = 16; 
    else
      toobig = 1;
  } else {
    index = 0;
    size_t temp = asize; /* So that we don't destroy original asize! by shifting */
    while (temp > 32) {
      temp >>= 1;
      index++;
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
