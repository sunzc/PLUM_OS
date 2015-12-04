#include<stdio.h>
#include<stdlib.h>

/**
 * malloc features
 * word size is 4 bytes, 2 words alignment
 * smallest assignment block size is 16 bytes
 * implicit free list, using header, footer, each take up 4 bytes
 * header and footer format as follows, they are all 8 bytes alignment, so the least 3
 * bits can be used to mark ALLOC status, 1 means alloced, 0 means free.
 *  31       3 2 1 0
 * | | |... | | | | |
 * |...SIZE...|ALLOC|
 */

/* borrow a lot from CSAPP */
#define WSIZE 4 /* word size */
#define DSIZE 8 /* double words size */
#define CHUNK_SIZE (1<<12) /* 4KB */

#define MAX(a, b) ((a) > (b) ? (a) : (b))


/* pack size with alloc status, 0 or 1 */
#define PACK(size,alloc) ((size) | (alloc))

/* read and write a word at addr p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* read the size and allocated field at addr p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x7)

/* Given block ptr bp, compute bp's address of header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) (FTRP(bp) + DSIZE)
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(bp - DSIZE))

#define ALIGN(size, align) ((size + align - 1) & (~(align - 1)))

static char *heap_listp; /* always point to the first block */
static int uninitialized_flag = 1;

static int minit(void);
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void *place(void *bp, size_t rsize);

extern void *sbrk(size_t increment);

/* create a heap with an initial free block */
static int minit()
{
	
	if ((heap_listp = sbrk(4*WSIZE)) == (void *)-1)
		return -1;
	PUT(heap_listp, 0);				/* Alignment padding */
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));	/* Prologue header */
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));	/* Prologue footer */
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));		/* Epilogue header */
	heap_listp += (2*WSIZE);

	/* extend the heap with a free block of CHUNK_SIZE bytes */
	if (extend_heap(CHUNK_SIZE/WSIZE) == NULL)
		return -1;
	
	uninitialized_flag = 0;
	return 0;
}

/* extend heap with a new free block */
static void *extend_heap(size_t words)
{
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if ((bp = sbrk(size)) == NULL)
		return NULL;

	//printf("[extend_heap]bp:0x%lx\n", bp);

	/* Initilize the free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));		/* Free block header */
	PUT(FTRP(bp), PACK(size, 0));		/* Free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	/* New epilogue header */

	/* coalesce if the previous block was free */
	return coalesce(bp);
}

/* coalesce adjact free block */
static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc){	/* case 1 */
		return bp;
	}

	else if (prev_alloc && !next_alloc){	/* case 2 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}

	else if (!prev_alloc && next_alloc){	/* case 3 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	else {
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
		     GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

/* find first fit for size */
static void *find_fit(size_t size)
{
	void *p = NEXT_BLKP(heap_listp);

	while(GET_SIZE(HDRP(p))) {
		if (GET_ALLOC(HDRP(p)) == 0 && 
			GET_SIZE(HDRP(p)) >= size)
			return p;
		else
			p = NEXT_BLKP(p);
	}
	return NULL;
}

/* place free block bp, split it when necessary*/
static void *place(void *bp, size_t rsize)
{
	size_t bsize = GET_SIZE(HDRP(bp));
	size_t nsize = bsize - rsize;


	/* if block size is larger than request size, then split it */
	if (bsize > rsize) {
		PUT(HDRP(bp + rsize), PACK(nsize, 0));
		PUT(FTRP(bp + rsize), PACK(nsize, 0));
	}

	PUT(HDRP(bp), PACK(bsize, 1));
	PUT(FTRP(bp), PACK(bsize, 1));
	return bp;
}

void *malloc(size_t size)
{
	void *bp;
	size_t asize; /* adjusted size */

	size_t extend_size;

	/* I know it's ugly. but ...*/
	if (uninitialized_flag) {
		if (minit()) {
			printf("ERROR: initialie malloc failed..\n");
			return NULL;
		}
	}

	/* Ignore spurious alloc */
	if (size == 0)
		return NULL;

	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = ALIGN(size + DSIZE, DSIZE);

	/* Search free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	/* No fit found, Get more memory and place the block */
	extend_size = MAX(asize, CHUNK_SIZE);
	if ((bp = extend_heap(extend_size)) == NULL) {
		return NULL;
	}
	place(bp, asize);
//	printf("sblibc:malloc! bp = %lx\n", bp);
	return bp;
}

void free(void *ptr)
{
	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
//	printf("sblibc:free! bp = %lx\n", ptr);
	coalesce(ptr);
}

