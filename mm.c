/*

malloc lab

explicit allocator, explicit free List - LIFO

Team Name:SWJungle_week06_10
Member 1 :Humyung Lee:paul93lee@gmail.com
Member 2 :JongWoo Han:JongwooHan@gmail.com
Using default tracefiles in ./traces/
Perf index = 42 (util) + 40 (thru) = 82/100

*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

// 상수, 매크로

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)  // 힙을 확장시킬 때 단위 설정. (12로 설정, 다른 숫자도 상관 없음)

#define MAX(x, y) ((x) > (y) ? (x): (y))

#define PACK(size, alloc) ((size) | (alloc))  // or 비트연산, 둘이 더해준다.

#define GET(p) (*(unsigned int *)(p))  // p 주소 내놔!
#define PUT(p, val) (*(unsigned int*)(p) = (val))  // p 주소에 value 입력해!

#define GET_SIZE(p) (GET(p) & ~0x7)  // p 주소를 갖고와서, p & 11111000 실행. size 반환!
#define GET_ALLOC(p) (GET(p) & 0x1)  // p 주소를 갖고와서, p & 00000001 실행. 할당되어 있으면 00000001 else 00000000

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define PRED(p) ((char *)(p))  // payload 시작점 다음블록에 이전 노드 저장.
#define SUCC(p) ((char *)(p) + WSIZE)  // payload 시작점에 다음 노드를 저장.



team_t team = {
    /* Team name */
    "SWJungle_week06_10",
    /* First member's full name */
    "Humyung Lee",
    /* First member's email address */
    "paul93lee@gmail.co",
    /* Second member's full name (leave blank if none) */
    "JongWoo Han",
    /* Second member's email address (leave blank if none) */
    "JongwooHan@gmail.co"
};

/* rounds up to the nearest multiple of ALIGNMENT */

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 전역변수

static char *heap_listp = NULL; // 편의상 힙 시작점.
static char *find_ptr = NULL; // root가 가리키는 노드.

// 순서대로

void add_free(char* ptr){
    char* succ;
    succ = GET(find_ptr);
    if (succ != 0){ // 루트에 연결 되어있는게 있을 때. // 루트가 가리키는 주소가 널이 아닐떄
        PUT(succ, ptr); // 첫 노드의 이전 항목에 지금 갱신되는 것을 넣어주고.
    }
    PUT(SUCC(ptr), succ); // ptr의 다음 노드를 첫번째 노드로 연결 시켜준다.
    PUT(find_ptr, ptr); // 루트가 가리키는 애를 새로들어온 애로 바꾼다.
}

void fix_link(char *ptr){ // fix과정은 무조건 연결을 끊어줌
    if(GET(PRED(ptr)) == NULL){ // 첫노드
        if(GET(SUCC(ptr)) != NULL){  // 다음 노드가 연결되어있으면,
            PUT(PRED(GET(SUCC(ptr))), 0); // 다음 노드의 주소의 이전 노드의 주소를 지운다.
        }
        PUT(find_ptr, GET(SUCC(ptr))); // 루트 노드가 첫 노드가 가리키던 다음 노드를 가리키게 한다.
	}
	else{ // 루트노드 이외에 다른 노드일 때
		if(GET(SUCC(ptr)) != NULL){ // 전, 후 모두 노드가 연결되어있으면
			PUT(PRED(GET(SUCC(ptr))), GET(PRED(ptr)));  // 다음노드의 주소의 이전노드값을 지금 노드의 이전값과 연결시킨다.
		}
		PUT(SUCC(GET(PRED(ptr))), GET(SUCC(ptr)));  // 이전 노드에 입력되어있던 다음 노드 주소에 지금 노드의 다음노드 주소를 넣어준다.
	}
	PUT(SUCC(ptr), 0); // 현재 노드의 SUCC, PRED 초기화
	PUT(PRED(ptr), 0);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size =  GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc){
        add_free(bp);
        return bp;
        // 둘다 할당 되어 있으면, free 리스트에 추가만 해주면 된다.
    }
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 다음 블록의 헤더를 보고 그 블록의 크기만큼 지금 블록의 사이즈에 추가한다.
        fix_link(NEXT_BLKP(bp)); // 다음 블록을 합쳐주고 초기화
        PUT(HDRP(bp), PACK(size,0)); // 헤더 갱신(더 큰 크기로 PUT)
        PUT(FTRP(bp), PACK(size,0)); // 푸터 갱신
    }
    else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        fix_link(PREV_BLKP(bp));// 이전 블록을 합쳐주고 초기화
        PUT(FTRP(bp), PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp); // block pointer 를 이전 블록으로 이동 시킨다.
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        fix_link(PREV_BLKP(bp));// 전블록
        fix_link(NEXT_BLKP(bp));// 다음블록
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        bp = PREV_BLKP(bp); // block pointer 를 이전 블록으로 이동 시킨다.
    }
    add_free(bp);
    return bp;
}

 static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words%2) ? (words+1) * WSIZE : words * WSIZE;

    if ( (long)(bp = mem_sbrk(size)) == -1){ // 올림.
        return NULL;
    }
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(PRED(bp), 0); // 힙을 늘려줄때 PRED, SUCC 생성.
    PUT(SUCC(bp), 0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
    return coalesce(bp);
}

int mm_init(void){
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*) -1) // 16바이트 만큼 확보한다. (unused + PH + PF + SUC + PRED + EH)
        return -1;
    PUT(heap_listp, 0);  // unused word 4 bytes, heap_listp 주소의 key값을 0으로 입력
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); // prologue header -> (8바이트(헤더푸터), 할당됨.)
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1)); // prologue footer생성.

    PUT(heap_listp + (3*WSIZE), PACK(0,1)); // 에필로그 블록헤더
    find_ptr = heap_listp;  // find_ptr 은 heap_listp의 주소값을 복사한다.
    heap_listp += (2*WSIZE);
    if (extend_heap(CHUNKSIZE/WSIZE)==NULL)
        return -1;
    return 0;
}

void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    PUT(SUCC(bp), 0);
    PUT(PRED(bp), 0);
    coalesce(bp);
}

static void *find_fit(size_t asize){ // first fit
    char *get_address = GET(find_ptr);

    while (get_address != NULL){
        if (GET_SIZE(HDRP(get_address)) >= asize){
            return get_address;
        }
        get_address = GET(SUCC(get_address));
    }
    return NULL; // not fit any
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp)); // 현재 블록
    fix_link(bp);
    if ( (csize-asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize,1));
        PUT(FTRP(bp), PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize,0));
        PUT(FTRP(bp), PACK(csize-asize,0));
        PUT(SUCC(bp), 0);
        PUT(PRED(bp), 0);
        coalesce(bp);
    }
    else{
        PUT(HDRP(bp), PACK(csize,1));
        PUT(FTRP(bp), PACK(csize,1));

    }
}

void *mm_malloc(size_t size){
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size <= 0) return NULL; // 0 보다 같거나 작으면 할당해 줄 필요 없다.
     asize = ALIGN(size + SIZE_T_SIZE);

    if ((bp = find_fit(asize)) != NULL){ //first fit
        place(bp,asize);
        return bp;
    }
    extendsize = MAX(asize,CHUNKSIZE);
    if ( (bp=extend_heap(extendsize/DSIZE)) == NULL){
        return NULL;  // 확장이 안되면 NULL 반환해라.
    }
    place(bp,asize); //  확장이 되면 넣어라.
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
