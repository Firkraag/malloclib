#include "stdint.h"
#include "string.h"
#include "assert.h"
#include "stdlib.h"
#include "config.h"
#include "memlib.h"
#include "mm.h"
#define FREE_LIST_ARRAY_SIZE (1024 + 10) 
static char *heap_listp;
static char *free_list_array[FREE_LIST_ARRAY_SIZE];
#define DSIZE 8  /* Double word size (bytes) */
#define QSIZE 16 /* Quad word size (bytes) */
// #define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLEAR_BIT(n, k) (n & ~(1 << k))
#define SET_K_BIT(n, k, bit_value) (CLEAR_BIT(n, k) | (bit_value << k))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(uint64_t *)(p))
#define PUT(p, val) (*(uint64_t *)(p) = (uint64_t)(val))
/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0xf)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define SET_SIZE(p, size) PUT(p, (GET(p) & 0xf) | (size))
#define SET_ALLOC(p, alloc) PUT(p, SET_K_BIT(GET(p), 0, alloc))
/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-DSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 2 * DSIZE)
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-DSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-2 * DSIZE)))
#define GET_PRED(bp) ((void *)GET(bp))
#define GET_SUCC(bp) ((void *)GET((bp) + DSIZE))
#define SET_PRED(bp, addr) PUT(bp, addr)
#define SET_SUCC(bp, addr) PUT(bp + DSIZE, addr)
team_t team = {
    /* Team name */
    "Sample allocator using implicit lists",
    /* First member's full name */
    "Godmar Back",
    "gback@cs.vt.edu",
    /* Second member's full name (leave blank if none) */
    "",
    "",
};
// static void all_free_block(char *free_list) {
//     printf("free block list is: \n");
//     if (free_list == NULL) {
//         return;
//     }

//     char *free_block = free_list;
//     while (1)
//     {
//         printf("%p\n", free_block);
//         if (SUCC_FREE_BLOCK(free_block) != free_block) {
//             free_block = SUCC_FREE_BLOCK(free_block);
//         }
//         else {
//             break;
//         }
//     }

// }
static size_t get_free_list_index(size_t size)
{
    if (size < 1024) {
        return size;
    }
    int pow = 1024;
    for (int i = 0; i < 10; i++)
    {
        if (size >= pow && pow * 2 > size)
        {
            return 1024;
        }
    }
    return FREE_LIST_ARRAY_SIZE - 1;
}
static void add_free_list(char *bp)
{

    size_t size = GET_SIZE(HDRP(bp));
    size_t free_list_index = get_free_list_index(size);
    //     if (size >= 10000)
    // {
    // printf("add free block %p\n", bp);
    // all_free_block(free_list_array[free_list_index]);
    // }
    char *free_list = free_list_array[free_list_index];
    SET_PRED(bp, 0);
    SET_SUCC(bp, free_list);
    if (free_list != NULL)
    {
        SET_PRED(free_list, bp);
    }
    free_list_array[free_list_index] = bp;
}
static void remove_free_list(void *bp)
{

    size_t size = GET_SIZE(HDRP(bp));
    size_t free_list_index = get_free_list_index(size);
    // // if (size >= 10000)
    // // {
    // // printf("before remove %p:\n", bp);
    // // all_free_block(free_list_array[free_list_index]);
    // // }
    char *succ_block = GET_SUCC(bp);
    char *pred_block = GET_PRED(bp);
    if (succ_block != NULL)
    {
        SET_PRED(succ_block, pred_block);
    }
    if (pred_block != NULL)
    {
        SET_SUCC(pred_block, succ_block);
    }
    else
    {
        free_list_array[free_list_index] = succ_block;
    }
}
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc)
    { /* Case 1 */
        add_free_list(bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    { /* Case 2 */
        remove_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_free_list(bp);
    }
    else if (!prev_alloc && next_alloc)
    { /* Case 3 */
        remove_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_free_list(bp);
    }
    else
    { /* Case 4 */
        remove_free_list(PREV_BLKP(bp));
        remove_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_free_list(bp);
    }
    return bp;
}
static void *extend_heap(size_t size)
{
    char *bp;

    /* Allocate an even number of double word to maintain alignment */
    if ((bp = mem_sbrk(size)) == NULL)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(DSIZE, 1)); /* New epilogue header */
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}
int mm_init(void)
{
    for (int i = 0; i < FREE_LIST_ARRAY_SIZE; i++)
    {
        free_list_array[i] = NULL;
    }

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * DSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                              /* Alignment padding */
    PUT(heap_listp + DSIZE, PACK(2 * DSIZE, 1));     /* Prologue header */
    PUT(heap_listp + 2 * DSIZE, PACK(2 * DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * DSIZE), PACK(0, 1));       /* Epilogue header */
    heap_listp += (2 * DSIZE);

    return 0;
}
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    remove_free_list(bp);
    if ((csize - asize) >= 2 * QSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        add_free_list(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
static void *find_fit(size_t asize)
{
    // for (int i = get_free_list_index(asize); i < FREE_LIST_ARRAY_SIZE; i++)
    // {
        char *free_list = free_list_array[get_free_list_index(asize)];
        for (void *bp = free_list; bp != NULL; bp = GET_SUCC(bp))
        {
            if (GET_SIZE(HDRP(bp)) >= asize)
            {
                return bp;
            }
        }
    // }

    return NULL;
}
void *mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    asize = QSIZE * ((size + QSIZE + (QSIZE - 1)) / QSIZE);
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) == NULL)
    {
        if ((bp = extend_heap(asize)) == NULL)
            return NULL;
    }

    place(bp, asize);
    return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
    size_t old_size = GET_SIZE(HDRP(ptr));
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    if (old_size - QSIZE >= size)
    {
        // place(ptr, size);
        return ptr;
    }
    size_t asize = QSIZE * ((size + QSIZE + (QSIZE - 1)) / QSIZE);
    char *bp;
    /* Search the free list for a fit */
    char *newptr = coalesce(ptr);
    if (GET_SIZE(HDRP(newptr)) >= size + QSIZE) {
        memcpy(newptr, ptr, old_size - QSIZE);
        return newptr;
    }
    if ((bp = find_fit(asize)) == NULL)
    {
        if ((bp = extend_heap(asize)) == NULL)
            return NULL;
    }
    place(bp, asize);
    memcpy(bp, ptr, old_size - QSIZE);
    PUT(HDRP(ptr), PACK(old_size, 0));
    PUT(FTRP(ptr), PACK(old_size, 0));
    coalesce(ptr);
    return bp;
}
