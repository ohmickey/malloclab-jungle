/*

malloc lab

explicit allocator, implicit List - next_fit

Team Name:SWJungle_week06_10
Member 1 :Humyung Lee:paul93lee@gmail.com
Member 2 :JongWoo Han:JongwooHan@gmail.com
Using default tracefiles in ./traces/
Perf index = 43 (util) + 40 (thru) = 83/100

*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

// 상수, 매크로

#define WSIZE 4  // 4bytes (word)
#define DSIZE 8  // 8bytes (double word)
#define CHUNKSIZE (1 << 12)  // 한번에 늘릴 사이즈 설정.

#define MAX(x, y) ((x) > y ? (x): (y))

#define PACK(size, alloc) ((size) | (alloc))  // or 비트연산, 둘이 더해준다.

#define GET(p) (*(unsigned int *)(p))  // p 주소 내놔!
#define PUT(p, val) (*(unsigned int*)(p) = (val))  // p 주소에 value 입력해!

#define GET_SIZE(p) (GET(p) & ~0x7)  // p 주소를 갖고와서, p & 11111000 실행. size 반환!
#define GET_ALLOC(p) (GET(p) & 0x1)  // p 주소를 갖고와서, p & 00000001 실행. 할당되어 있으면 00000001 else 00000000

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

team_t team = {
    /* Team name */
    "SWJungle_week06_10",
    /* First member's full name */
    "Humyung Lee",
    /* First member's email address */
    "paul93lee@gmail.com",
    /* Second member's full name (leave blank if none) */
    "JongWoo Han",
    /* Second member's email address (leave blank if none) */
    "JongwooHan@gmail.com"
};

#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp;
static char *find_ptr;

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size =  GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        find_ptr = bp;
        return bp;
    }
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp), PACK(size,0));
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp);
    }
    find_ptr = bp;
    return bp;
}

 static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    size = (words%2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

    return coalesce(bp);
}

int mm_init(void){
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*) -1){
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1));
    PUT(heap_listp + (3*WSIZE), PACK(0,1));
    heap_listp+= (2*WSIZE);
    find_ptr = heap_listp;
    if (extend_heap(CHUNKSIZE/WSIZE)==NULL)
        return -1;
    return 0;
}

void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    coalesce(bp);
}

static void *find_fit(size_t asize){
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if (!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}
static void *next_fit(size_t asize){ // next fit 검색
    void *bp;
    for (bp = NEXT_BLKP(find_ptr); GET_SIZE(HDRP(bp)) >= 0; bp = NEXT_BLKP(bp)){
        if (GET_SIZE(HDRP(bp)) == 0){ // 0을 만나면(epilogue header), 힙의 시작 위치에서 다시탐색.
            bp = heap_listp;
        }
        if (!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))){
            find_ptr = bp;
            return bp;
        }
        if (bp == find_ptr){ // find_ptr 까지 왔는데 검색이 안됐으면 맞는 사이즈가 없으니 return
            return NULL;
        }
    }
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    if ( (csize-asize) >= (2*DSIZE)){  // split 이 가능한지 확인
        PUT(HDRP(bp), PACK(asize,1));
        PUT(FTRP(bp), PACK(asize,1));
        bp = NEXT_BLKP(bp); //split 이후 free 블록으로 나눠질 블록의 bp로 이동.
        PUT(HDRP(bp), PACK(csize-asize,0)); // free header
        PUT(FTRP(bp), PACK(csize-asize,0)); // free footer
    }
    else{
        PUT(HDRP(bp), PACK(csize,1)); // split 이 안되면 모두 할당
        PUT(FTRP(bp), PACK(csize,1));
    }
}

void *mm_malloc(size_t size){
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size =< 0) return NULL;

    if (size <= DSIZE){
        asize = 2*DSIZE;
    }
    else {
        asize = DSIZE* ( (size + (DSIZE)+ (DSIZE-1)) / DSIZE );
    }
    if ((bp = next_fit(asize)) != NULL){ // next_fit 탐색.
        place(bp,asize); // 탐색 성공시 위치시킨다.
        return bp;
    }
    // 들어갈 곳이 없으면, extend size.
    extendsize = MAX(asize,CHUNKSIZE);
    if ( (bp=extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp,asize);
    return bp;
}

void *mm_realloc(void *bp, size_t size){
    if(size <= 0){
        mm_free(bp);
        return 0;
    }
    if(bp == NULL){
        return mm_malloc(size);
    }
    void *newptr = mm_malloc(size);
    if(newptr == NULL){
        return 0;
    }
    size_t oldsize = GET_SIZE(HDRP(bp));
    if(size < oldsize){
    	oldsize = size;
	}
    memcpy(newptr, bp, oldsize);
    mm_free(bp);
    return newptr;
}
