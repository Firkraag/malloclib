#include "stdint.h"
#include "string.h"
#include "assert.h"
#include "stdlib.h"
#include "config.h"
#include "memlib.h"
#include "mm.h"
// static void *heap_end;
// static int32_t *start_block;
// static int32_t *end_block;
#define FREE_LIST_ARRAY_SIZE 5
static char *heap_listp;
static char *free_list_array[FREE_LIST_ARRAY_SIZE];
#define GET_BLOCK_SIZE(block) ((block)[0] & 0xfffffffe)
#define GET_BLOCK_ALLOCATE(block) ((block)[0] & 0x1)
#define SET_BLOCK_SIZE(block, size) ((block)[0] = GET_BLOCK_ALLOCATE((block)) | (size))
// #define SET_BLOCK_ALLOCATE(block, flag) ((block)[0] ^= (-(flag) ^ (block)[0]) & 1)
#define SET_BLOCK_ALLOCATE(block, flag) ((block)[0] = GET_BLOCK_SIZE(block) | (flag))
/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define CLEAR_BIT(n, k) (n & ~(1 << k))
#define SET_K_BIT(n, k, bit_value) (CLEAR_BIT(n, k) | (bit_value << k))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))
/* Read and write a word at address p */
#define GET(p) (*(uint64_t *)(p))
#define PUT(p, val) (*(uint64_t *)(p) = (uint64_t) (val))
/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0xf)
#define GET_ALLOC(p) (GET(p) & 0x1)
// #define GET_FRONT(p) ((GET(p) & 0x2) >> 1)
#define SET_SIZE(p, size) PUT(p, (GET(p) & 0xf) | (size))
#define SET_ALLOC(p, alloc) PUT(p, SET_K_BIT(GET(p), 0, alloc))
// #define SET_FRONT(p, front) PUT(p, SET_K_BIT(GET(p), 1, front))
/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-DSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - 2 * DSIZE)
/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-DSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)- 2 * DSIZE)))
#define GET_PRED(bp) ((void *) GET(bp))
#define GET_SUCC(bp) ((void *) GET((bp) + DSIZE))
#define SET_PRED(bp, addr) PUT(bp, addr)
#define SET_SUCC(bp, addr) PUT(bp + DSIZE, addr)
// #define SUCC_FREE_BLOCK(bp) (GET_FRONT(FTRP(bp)) ? (bp - GET(SUCC(bp))) : (bp + GET(SUCC(bp))))
// #define PRED_FREE_BLOCK(bp) (GET_FRONT(HDRP(bp)) ? (bp - GET(PRED(bp))) : (bp + GET(PRED(bp))))
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
    return MIN(size / 1000, FREE_LIST_ARRAY_SIZE - 1);
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
    if (free_list != NULL) {
        SET_PRED(free_list, bp);
    }
    free_list_array[free_list_index] = bp;
    // if (free_list != NULL)
    // {
    //     if (free_list > bp)
    //     {
    //         PUT(SUCC(bp), free_list - bp);
    //         PUT(PRED(free_list), free_list - bp);
    //         SET_FRONT(HDRP(free_list), 1);
    //         SET_FRONT(FTRP(bp), 0);
    //         // assert(GET(PRED(free_list)) == free_list - bp);
    //         // assert(GET(SUCC(bp)) == free_list - bp);
    //         // assert(GET_FRONT(HDRP(free_list)) == 1);
    //         // assert(GET_FRONT(FTRP(bp)) == 0);
    //         // assert(SUCC_FREE_BLOCK(bp) == free_list);
    //         // assert(PRED_FREE_BLOCK(free_list) == bp);
    //     }
    //     else
    //     {
    //         PUT(SUCC(bp), bp - free_list);
    //         PUT(PRED(free_list), bp - free_list);
    //         SET_FRONT(HDRP(free_list), 0);
    //         SET_FRONT(FTRP(bp), 1);
    //         // assert(GET(PRED(free_list)) == bp - free_list);
    //         // assert(GET(SUCC(bp)) == bp - free_list);
    //         // assert(GET_FRONT(HDRP(free_list)) == 0);
    //         // assert(GET_FRONT(FTRP(bp)) == 1);
    //         // assert(SUCC_FREE_BLOCK(bp) == free_list);
    //         // assert(PRED_FREE_BLOCK(free_list) == bp);
    //     }
    // }
    // if (size >= 10000)
    // {
    // printf("after add: \n");
    // all_free_block(free_list_array[free_list_index]);
    // }

    // all_free_block();
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
    if (succ_block != NULL) {
        SET_PRED(succ_block, pred_block);
    }
    if (pred_block != NULL) {
        SET_SUCC(pred_block, succ_block);
    }
    else {
        free_list_array[free_list_index] = succ_block;
    }
    // if (pred_block != bp) {
    //     if (succ_block == bp) {
    //         PUT(SUCC(pred_block), 0);
    //     }
    //     else {
    //         PUT(SUCC(pred_block), abs(succ_block - pred_block));
    //     }
    // }
    // if (succ_block != bp) {
    //     if (pred_block == bp) {
    //         PUT(PRED(succ_block), 0);
    //     }
    //     else {
    //         PUT(PRED(succ_block), abs(succ_block - pred_block));
    //     }
    // }
    // if (pred_block < succ_block) {
    //     SET_FRONT(FTRP(pred_block), 0);
    //     SET_FRONT(HDRP(succ_block), 1);
    // }
    // else if (pred_block > succ_block) {
    //     SET_FRONT(FTRP(pred_block), 1);
    //     SET_FRONT(HDRP(succ_block), 0);
    // }
    // if (pred_block == bp) {
    //     if (succ_block == bp) {
    //         free_list_array[free_list_index] = NULL;
    //     }
    //     else {
    //         free_list_array[free_list_index] = succ_block;
    //     }
    // }
    // // if (size >= 10000)
    // // {
    // // printf("after remove:\n");
    // // all_free_block(free_list_array[free_list_index]);
    // // }
    // // if (succ_block == bp) {
    // //     PUT(SUCC(pred_block), 0);
    // // }
    // // else {
    // //     if (pred_block < succ_block)
    // //     {
    // //         PUT(SUCC(pred_block), succ_block - pred_block);
    // //         SET_FRONT(FTRP(pred_block), 0);
    // //         PUT(PRED(succ_block), succ_block - pred_block);
    // //         SET_FRONT(HDRP(succ_block), 1);
    // //     }
    // //     else {
    // //         PUT(SUCC(pred_block), pred_block - succ_block);
    // //         SET_FRONT(FTRP(pred_block), 1);
    // //         PUT(PRED(succ_block), pred_block - succ_block);
    // //         SET_FRONT(HDRP(succ_block), 0);
    // //     }
    // // }
    // // if (pred_block == bp) {
    // //     PUT(PRED(succ_block), 0);
    // //     free_list = pred_block;
    // // }
    // // else {
    // //     if (pred_block < succ_block)
    // //     {
    // //         PUT(PRED(succ_block), succ_block - pred_block);
    // //         SET_FRONT(HDRP(succ_block), 1);
    // //     }
    // //     else {
    // //         PUT(PRED(succ_block), pred_block - succ_block);
    // //         SET_FRONT(HDRP(succ_block), 0);
    // //     }

    // // }
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
static void *extend_heap(size_t double_word)
{
    char *bp;
    size_t size;

    /* Allocate an even number of double word to maintain alignment */
    size = (double_word % 2) ? (double_word + 1) * DSIZE : double_word * DSIZE;
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
    if ((heap_listp = mem_sbrk(8 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                                /* Alignment padding */
    PUT(heap_listp + DSIZE, PACK(2 * DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + 2 * DSIZE, PACK(2 * DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * DSIZE), PACK(0, 1));         /* Epilogue header */
    heap_listp += (2 * DSIZE);

    // /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    // if (extend_heap(CHUNKSIZE / DSIZE) == NULL)
    //     return -1;
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
    if ((csize - asize) >= (4 * DSIZE))
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
    char *free_list = free_list_array[get_free_list_index(asize)];
    // if (asize >= 10000) {
    //     all_free_block(free_list);
    // }
    for (void *bp = free_list; bp != NULL; bp = GET_SUCC(bp)) {
        if (GET_SIZE(HDRP(bp)) >= asize)
        {
            return bp;
        }
    }
    // while (1)
    // {
    //     if (GET_SIZE(HDRP(bp)) >= asize)
    //     {
    //         return bp;
    //     }
    //     if (bp == SUCC_FREE_BLOCK(bp))
    //     {
    //         break;
    //     }
    //     bp = SUCC_FREE_BLOCK(bp);
    // }
    return NULL;
    // for (bp = free_list; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    // {
    //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
    //         return bp;
    //     }
    // }
    // return NULL; /* No fit */
}
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    if (heap_listp == 0)
    {
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= 2 * DSIZE)
        asize = 4 * DSIZE;
    else
        asize = 2 * DSIZE * ((size + (2 * DSIZE) + (2 * DSIZE - 1)) / (2 * DSIZE));

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) == NULL)
    {
        // extendsize = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(asize / DSIZE)) == NULL)
            return NULL;
    }

    place(bp, asize);
    return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
    // int required_size = (size / (2 * DSIZE) ) * 2 * DSIZE;
    int old_size = GET_SIZE(HDRP(ptr));
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }
    if (old_size - 2 * DSIZE >= size)
    {
        return ptr;
    }
    char *pos = mm_malloc(size);
    memcpy(pos, ptr, old_size - 2 * DSIZE);
    mm_free(ptr);
    return pos;
}

// int main(void) {
//     unsigned int a = 0xff;
//     unsigned int b = 0xfe;
//     printf("%#x\n", CLEAR_BIT(a, 0));
//     printf("%#x\n", CLEAR_BIT(a, 1));
//     printf("%#x\n", CLEAR_BIT(a, 2));
//     printf("%#x\n", SET_K_BIT(0, 0, 1));
//     printf("%#x\n", SET_K_BIT(0, 1, 1));
//     printf("%#x\n", SET_K_BIT(0, 2, 1));
//     printf("%#x\n", SET_K_BIT(1, 2, 1));
//     printf("%#x\n", SET_ALLOC(&a, 0));
//     printf("%#x\n", SET_ALLOC(&a, 1));
//     printf("%#x\n", SET_ALLOC(&b, 0));
//     printf("%#x\n", SET_ALLOC(&b, 1));
//     printf("%#x\n", SET_FRONT(&a, 0));
//     printf("%#x\n", GET_FRONT(&a));
//     printf("%#x\n", SET_FRONT(&a, 1));
//     printf("%#x\n", GET_FRONT(&a));
//     printf("%#x\n", SET_FRONT(&b, 0));
//     printf("%#x\n", GET_FRONT(&b));
//     printf("%#x\n", SET_FRONT(&b, 1));
//     printf("%#x\n", GET_FRONT(&b));
//     unsigned int c = 0;
//     printf("%#x\n", SET_SIZE(&c, 16));
//     printf("%#x\n", SET_SIZE(&c, 32));
//     c = 1;
//     printf("%#x\n", SET_SIZE(&c, 32));
// }

// int mm_init(void)
// {
//     mem_init(0);
//     char *heap_start = mem_sbrk(0);
//     if ((uintptr_t)(heap_start + 4) % ALIGNMENT)
//     {
//         int padding = ALIGNMENT - (((uintptr_t)(heap_start + 4)) % ALIGNMENT);
//         start_block = mem_sbrk(padding + 4) + padding;
//     }
//     else
//     {
//         start_block = (int32_t *)heap_start;
//     }
//     end_block = start_block;
//     return 0;
// }
// void *mm_malloc(size_t size)
// {
//     for (int32_t *block = start_block; block < end_block; block = (int32_t *)((char *)block + GET_BLOCK_SIZE(block)))
//     {
//         if (!GET_BLOCK_ALLOCATE(block) && GET_BLOCK_SIZE(block) >= size)
//         {
//             SET_BLOCK_ALLOCATE(block, 1);
//             return block + 4;
//         }
//     }
//     int allocated_size = (((size + 4) / ALIGNMENT) + 1) * ALIGNMENT;
//     mem_sbrk(allocated_size);
//     SET_BLOCK_SIZE(end_block, allocated_size);
//     SET_BLOCK_ALLOCATE(end_block, 1);
//     end_block = (int32_t *)((char *)end_block + allocated_size);
//     return (char *)end_block - allocated_size + 4;
// }
// void mm_free(void *ptr)
// {
//     char *block = (char *)ptr;
//     // char *next_block = block + GET_BLOCK_SIZE(block);
//     // if (next_block != (char *) end_block && GET_BLOCK_ALLOCATE((int32_t *) next_block)) {
//     //     SET_BLOCK_SIZE(block, GET_BLOCK_SIZE(block)+ GET_BLOCK_SIZE(next_block));
//     // }
//     SET_BLOCK_ALLOCATE((int32_t *)block, 0);
// }