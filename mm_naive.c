#include "stdint.h"
#include "config.h"
#include "memlib.h"
#include "mm.h"
// static void *heap_end;
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
int mm_init(void)
{
    mem_init(0);
    return 0;
}
void *mm_malloc (size_t size) {

    void *start = mem_sbrk(0);
    if (((uintptr_t)start) % ALIGNMENT != 0)
    {
        int padding = ALIGNMENT - (((uintptr_t)start) % ALIGNMENT);
        mem_sbrk(size + padding);
        return start + padding;
    }
    return mem_sbrk(size);
}
void mm_free (void *ptr) {

}
void *mm_realloc(void *ptr, size_t size) {
    return NULL;
}

