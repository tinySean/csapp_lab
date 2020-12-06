# CSAPP-LAB5

``malloc lab``
通过填写三个函数mm_init, mm_malloc, mm_free, mm_realloc来实现malloc功能

## 隐式空闲链表法
直接参考书上例子，具体code可以在官网上找到，直接复制过来就行

## 显示空闲链表法
块 存在两种状态 ： 一是free，二是busy
对于free的块，要维护一个双向链表，其中的两个指针域复用了payload
对于busy的块，则像隐式空闲链表法一样修改头部和脚部即可

主要在隐式空闲链表法的基础上进行修改：
加入链表头:
```c
static unsigned int *free_litsp = 0;
```
加入一些宏函数:
```c
#define GET_PREV_PTR(bp) (*(unsigned int *)(bp))
#define GET_NEXT_PTR(bp) (*((unsigned int *)(bp) + 1))
#define SET_PREV_PTR(bp, val) (*(unsigned int *)(bp) = (val))
#define SET_NEXT_PTR(bp, val) (*((unsigned int *)(bp) + 1) = (val))
```
加入链表的插入和删除函数:
```c
// 采用头插法
static void insert_to_free_list(void* bp)
{
    if (bp == NULL)
        return;
    SET_PREV_PTR(bp, 0); // 清空bp指针之前的指向，这个很重要，因为如果bp不为空闲快的话，bp->prev还是存在内容的，之后会因为这个出错
    SET_NEXT_PTR(bp, free_litsp);
    if (free_litsp != NULL)
    {
        SET_PREV_PTR(free_litsp, bp);
    }
    free_litsp = bp;
}

static void remove_from_free_list(void *bp){
    // 删除不需要清空bp的前后指向，因为它会自动被新内容覆盖
    if(bp == NULL || GET_ALLOC(HDRP(bp)))
        return;
    void* prev = GET_PREV_PTR(bp);
    void* next = GET_NEXT_PTR(bp);
    if(prev == 0 && next == 0){
        free_litsp = 0;
    }else if(prev == 0 && next != 0){
        SET_PREV_PTR(next, 0);
        free_litsp = next;
    }else if(prev != 0 && next == 0){
        SET_NEXT_PTR(prev, 0);
    }else{
        SET_NEXT_PTR(prev, next);
        SET_PREV_PTR(next, prev);
    }        
}
```
合并的时候需要注意释放对应的空闲块，并且需要将合并后的空闲快加入到空闲链表中
```c
static void *coalesce(void *bp) 
{
    char* nbp = NEXT_BLKP(bp);
    char* pbp = PREV_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(FTRP(pbp));
    size_t next_alloc = GET_ALLOC(HDRP(nbp));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        remove_from_free_list(nbp);
        size += GET_SIZE(HDRP(nbp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        remove_from_free_list(pbp);
        size += GET_SIZE(HDRP(pbp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
        remove_from_free_list(nbp);
        remove_from_free_list(pbp);
        size += GET_SIZE(HDRP(nbp)) + GET_SIZE(FTRP(pbp));
        PUT(HDRP(pbp), PACK(size, 0));
        PUT(FTRP(nbp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    insert_to_free_list(bp);
    return bp;
}
```
在放置的时候也需要删除对应的空闲快，和放入新空闲块
```c
static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    remove_from_free_list(bp);
    size_t csize = GET_SIZE(HDRP(bp));   
    if ((csize - asize) >= (2*DSIZE)) { 
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        char* nbp = NEXT_BLKP(bp);
        PUT(HDRP(nbp), PACK(csize-asize, 0));
        PUT(FTRP(nbp), PACK(csize-asize, 0));
        insert_to_free_list(nbp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
```

## 离散空闲链表法
离散空闲链表的可以理解为 多条空闲链表，每条空闲链表有一定的 SIZE限制
在上面的基础上，主要修改如下：
```c
/*
* get_freelisthead - get the free list's head pointer fit the given size.
*/
void* get_freelisthead(size_t size)
{
    int i = 0;
    if (size <= 32) i=0;
    else if (size <= 64) i=1;
    else if (size <= 128) i=2;
    else if (size <= 256) i=3;
    else if (size <= 512) i=4;
    else if (size <= 1024) i=5;
    else if (size <= 2048) i=6;
    else if (size <= 4096) i=7;
    else  i=8;

    return block_list_start+(i*WSIZE);
}

/*
 * remove_from_free_list - remove the block from free list.
 */ 
static void remove_from_free_list(void *bp)
{
    if (bp == NULL || GET_ALLOC(HDRP(bp)))
        return;
    void *root = get_freelisthead(GET_SIZE(HDRP(bp))); 
    void* prev = GET_PREV(bp);
    void* next = GET_NEXT(bp);

    if (prev == NULL)
    {
        if (next != NULL) SET_PREV(next, NULL);
        PUT(root, next);
    }
    else
    {
        if (next != NULL) SET_PREV(next, prev);
        SET_NEXT(prev, next);
    }
}

/* 
 * insert_to_free_list - insert block bp into segragated free list.
*   In each category the free list is ordered by the free size for small to big.
*   When find a fit free block ,just find for begin to end ,the first fit one is the best fit one.
 */
static void insert_to_free_list(void* bp)
{
    if (bp == NULL)
        return;
    void* root = get_freelisthead(GET_SIZE(HDRP(bp)));
    void* prev = root;
    void* next = GET(root);

    while (next != NULL)
    {
        if (GET_SIZE(HDRP(next)) >= GET_SIZE(HDRP(bp))) break;
        prev = next;
        next = GET_NEXT(next);
    }

    if (prev == root)
    {
        PUT(root, bp);
        SET_PREV(bp, NULL);
        SET_NEXT(bp, next);
        if (next != NULL) SET_PREV(next, bp);
    }
    else
    {
        SET_PREV(bp, prev);
        SET_NEXT(bp, next);
        SET_NEXT(prev, bp);
        if (next != NULL) SET_PREV(next, bp);
    }
}
static void* find_fit(size_t asize)
{
    for (void* root = get_freelisthead(asize); root != (heap_listp-WSIZE); root+=WSIZE)
    {
        void* bp = GET(root);
        while (bp)
        {
            if (GET_SIZE(HDRP(bp)) >= asize) return bp;
            bp = GET_NEXT(bp);
        }
    }
    return NULL;
}
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(12*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);           //block size <= 32   
    PUT(heap_listp+(1*WSIZE), 0); //block size <= 64
    PUT(heap_listp+(2*WSIZE), 0); //block size <= 128
    PUT(heap_listp+(3*WSIZE), 0); //block size <= 256
    PUT(heap_listp+(4*WSIZE), 0); //block size <= 512
    PUT(heap_listp+(5*WSIZE), 0); //block size <= 1024
    PUT(heap_listp+(6*WSIZE), 0); //block size <= 2048
    PUT(heap_listp+(7*WSIZE), 0); //block size <= 4096
    PUT(heap_listp+(8*WSIZE), 0); //block size > 4096
    PUT(heap_listp+(9*WSIZE), PACK(DSIZE, 1)); //prologue header
    PUT(heap_listp+(10*WSIZE), PACK(DSIZE, 1)); //prologue footer
    PUT(heap_listp+(11*WSIZE), PACK(0, 1)); //epilogue header

    block_list_start = heap_listp;
    heap_listp += (10 * WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - find a free block in free list ,if there is no one exists, exthed the heap 
 *
 */
void *mm_malloc(size_t size)
{
    size_t asize; // adjusted block size
    size_t extendsize; // amount to extend heap if no fit
    char* bp;

    if (size == 0)
        return NULL;

    // adjusted block size to include overhead and alignment reqs
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    // no fit found, get more memory and place the block
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}
```