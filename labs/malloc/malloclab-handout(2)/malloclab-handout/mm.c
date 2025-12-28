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

typedef struct island_t {
  void* next_island;
  int size;
  int padding;
} island_t;

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
  island_header->next_island = NULL;
  island_header->size = current_avail_size;
  island_header->padding = 0;
  current_avail = (void*)((char*)current_avail + sizeof(island_header));
  current_avail_size -= sizeof(island_header);

  /* store start of freelist array */
  freeArrPtr = current_avail;

  int listqty = 27;
  current_avail = (void*)((char*)current_avail + listqty*8);
  current_avail_size -= listqty*8;

  /*read the next 8 bytes as ptr then deref that and set size, flags*/
  /*hence make epilogue block and also store info abt predecessor block*/
  *(size_t*)current_avail = 3; /* set the prevalloc and alloc flags and set size = 0 */

  /*Initialise the seg freelist to all NULLs*/
  void* bp = freeArrPtr;                // OR:- 
  for (int i = 0; i < listqty; i++) {   // void **seg_list = (void**)freeArrPtr; 
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
  int newsize = ALIGN(size);
  void *p;
  
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
