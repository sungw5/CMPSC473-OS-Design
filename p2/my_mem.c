/**
 * File             : my_mem.c
 * Description      : Project 2: Memory Allocation
 * Author(s)        : Sung Woo Oh, Jihoon (Jeremiah) Park
*/

// Include files
#include <stdio.h>
#include <stdlib.h>
#define N_OBJS_PER_SLAB 64
#define SPLIT_SIZE 11

// Functional prototypes
void setup(int malloc_type, int mem_size, void *start_of_memory);
void *my_malloc(int size);
void my_free(void *ptr);

// struct
typedef struct block
{
    int start;
    int end;
    int size;
    void *address;
    struct block *next;
} block;

typedef struct slabblock
{
    int start;
    int end;
    void *address;
} slabblock;

//blockinfoSLAB
typedef struct slab
{
    int start;
    int end;
    int size;
    struct slab *next;
} slab;

//slabinfos
typedef struct slabinfos
{
    int type;
    int start;
    int end;
    int size;
    int used;
    struct slabblock arrayslab[64]; //start end
    struct slabinfos *next;
} slabinfos;

//////////////////////////////////////////
//////////// global variables ////////////
//////////////////////////////////////////

block *blockinfo[SPLIT_SIZE];
block *allocated;

slab *blockinfoSLAB[SPLIT_SIZE];
slabinfos *slabinfo;

int option = -1;
void *memloc = 0;
int memsize = 0;

////////////////////////////////////////////////////////////////////////////
//
// Function     : Helper functions
//

void sorting(block *ptr)
{
    block *prev = ptr;
    block *temp = malloc(sizeof(block));
    if (ptr->next == NULL)
    {
        //do nothing
    }
    else
    {
        block *curr = ptr->next;
        while (curr != NULL)
        {
            if (curr->start < prev->start)
            {
                temp->start = prev->start;
                temp->end = prev->end;
                temp->size = prev->size;
                temp->address = NULL;

                prev->start = curr->start;
                prev->end = curr->end;
                prev->size = curr->size;
                prev->address = NULL;

                curr->start = temp->start;
                curr->end = temp->end;
                curr->size = temp->size;
                curr->address = NULL;

                prev = ptr;
                curr = ptr->next;
            }
            else
            {
                prev = prev->next;
                curr = curr->next;
            }
        }
    }
}

void sorting2(slab *ptr)
{
    slab *prev = ptr;
    slab *temp = malloc(sizeof(block));
    if (ptr->next == NULL)
    {
        //do nothing
    }
    else
    {
        slab *curr = ptr->next;
        while (curr != NULL)
        {
            if (curr->start < prev->start)
            {
                temp->start = prev->start;
                temp->end = prev->end;
                temp->size = prev->size;

                prev->start = curr->start;
                prev->end = curr->end;
                prev->size = curr->size;

                curr->start = temp->start;
                curr->end = temp->end;
                curr->size = temp->size;

                prev = ptr;
                curr = ptr->next;
            }
            else
            {
                prev = prev->next;
                curr = curr->next;
            }
        }
    }
}

void sortingSLAB(slabinfos *ptr)
{
    slabinfos *prev = ptr;
    slabinfos *temp = malloc(sizeof(slabinfos));
    if (ptr->next == NULL)
    {
        //do nothing
    }
    else
    {
        slabinfos *curr = ptr->next;
        while (curr != NULL)
        {
            if (curr->start < prev->start)
            {
                temp->type = prev->type;
                temp->start = prev->start;
                temp->end = prev->end;
                temp->size = prev->size;
                temp->used = prev->used;
                for (int i = 0; i < 64; i++)
                {
                    temp->arrayslab[i].start = prev->arrayslab[i].start;
                    temp->arrayslab[i].end = prev->arrayslab[i].end;
                    temp->arrayslab[i].address = prev->arrayslab[i].address;
                }

                prev->type = curr->type;
                prev->start = curr->start;
                prev->end = curr->end;
                prev->size = curr->size;
                prev->used = curr->used;
                for (int i = 0; i < 64; i++)
                {
                    prev->arrayslab[i].start = curr->arrayslab[i].start;
                    prev->arrayslab[i].end = curr->arrayslab[i].end;
                    prev->arrayslab[i].address = curr->arrayslab[i].address;
                }

                curr->type = temp->type;
                curr->start = temp->start;
                curr->end = temp->end;
                curr->size = temp->size;
                curr->used = temp->used;
                for (int i = 0; i < 64; i++)
                {
                    curr->arrayslab[i].start = temp->arrayslab[i].start;
                    curr->arrayslab[i].end = temp->arrayslab[i].end;
                    curr->arrayslab[i].address = temp->arrayslab[i].address;
                }
                prev = ptr;
                curr = ptr->next;
            }
            else
            {
                prev = prev->next;
                curr = curr->next;
            }
        }
    }
}

//blockinfo(ptr) -> allocated(tail)  (for Buddy system only)
void saveAllocated(block *ptr, void *address, int startingval)
{
    if (allocated->size == 0)
    {
        allocated->start = ptr->start;
        allocated->end = ptr->end;
        allocated->size = ptr->size;
        allocated->address = address + startingval;
    }
    else
    {
        block *newAllocated = malloc(sizeof(block));
        block *tail = allocated;
        while (tail->next != NULL)
        {
            tail = tail->next;
        }
        newAllocated->start = ptr->start;
        newAllocated->end = ptr->end;
        newAllocated->size = ptr->size;
        newAllocated->address = address + startingval;
        tail->next = newAllocated;
    }
}

//blockinfoSLAB(ptr) -> slab
int saveSLAB(slab *ptr, int type, void *address, int startingval)
{
    // 젤 첨에 아무거도 없을때
    int slabaddress = 0;
    if (slabinfo->size == 0)
    {
        slabinfo->type = type;
        slabinfo->start = ptr->start;
        slabinfo->end = ptr->end;
        slabinfo->size = ptr->size; //128kb
        slabinfo->used = 1;
        slabinfo->arrayslab[0].address = address + startingval;
        slabinfo->arrayslab[0].start = ptr->start;
        slabinfo->arrayslab[0].end = ptr->start + type - 1;
        slabaddress = slabinfo->arrayslab[0].start;
    }
    // 뭐가 있을때
    else if (slabinfo->size != 0)
    {
        slabinfos *temp = slabinfo;
        while (temp != NULL)
        {

            if (temp->type != type || temp->used == 64)
            {
                if ((temp->used < 64) && (temp->type == type))
                {
                    int i = 0;
                    while (temp->arrayslab[i].address != NULL)
                    {
                        i++;
                    }
                    // 빈공간에 그 자리에서 address + startingval 넣어주기;
                    if (temp->arrayslab[i].address == NULL)
                    {
                        temp->arrayslab[i].address = address + startingval;
                        temp->arrayslab[i].start = ptr->start;
                        temp->arrayslab[i].end = ptr->start + type - 1;
                        slabaddress = temp->arrayslab[0].start;
                        temp->used += 1;
                    }
                }

                // Used 64 다쓰면 새로 만들어주기
                else if ((temp->used < 64) && (temp->type != type))
                {
                    slabinfos *newAllocated = malloc(sizeof(slabinfos)); // 새로 만들어준거
                    slabinfos *tail = slabinfo;                          // 오리지날 포인터
                    while (tail->next != NULL)
                    {
                        tail = tail->next;
                    }
                    newAllocated->type = type;
                    newAllocated->used += 1;
                    newAllocated->start = ptr->start;
                    newAllocated->end = ptr->end;
                    newAllocated->size = ptr->size;
                    newAllocated->arrayslab[0].address = address + startingval;
                    newAllocated->arrayslab[0].start = ptr->start;
                    newAllocated->arrayslab[0].end = ptr->start + type - 1;
                    slabaddress = newAllocated->arrayslab[0].start;

                    tail->next = newAllocated; // 새로 만들어준거 오리지날 뒤에 붙이기
                }
                else if (temp->used == 64)
                {
                    slabinfos *newAllocated = malloc(sizeof(slabinfos)); // 새로 만들어준거
                    slabinfos *tail = slabinfo;                          // 오리지날 포인터
                    while (tail->next != NULL)
                    {
                        tail = tail->next;
                    }
                    newAllocated->type = type;
                    newAllocated->used += 1;
                    newAllocated->start = ptr->start;
                    newAllocated->end = ptr->end;
                    newAllocated->size = ptr->size;
                    newAllocated->arrayslab[0].address = address + startingval;
                    newAllocated->arrayslab[0].start = ptr->start;
                    newAllocated->arrayslab[0].end = ptr->start + type - 1;
                    slabaddress = newAllocated->arrayslab[0].start;

                    tail->next = newAllocated; // 새로 만들어준거 오리지날 뒤에 붙이기
                }
                break;
            }
            // Type 이 다를때 넘기기
            else
            {
                temp = temp->next;
            }
        }
    }
    sortingSLAB(slabinfo);
    return slabaddress;
}

int findnearest2spower(int size)
{
    int nearestval = 1024;
    // if size = 1234 -> nearestval should be 2048 (2^11)
    // if size = 4321 -> nearestval should be 8196 (2^13)
    while (size > nearestval)
    {
        nearestval = nearestval * 2;
    }
    return nearestval;
}

////////////////////////////////////////////////////////////////////////////
//
// Function     : setup
// Description  : initialize the memory allocation system
//
// Inputs       : malloc_type - the type of memory allocation method to be used [0..3] where
//                (0) Buddy System
//                (1) Slab Allocation

void setup(int malloc_type, int mem_size, void *start_of_memory)
{
    // int i = 0;
    // blockinfo[0] =  1kb and blockinfo[10] = 1024kb

    switch (malloc_type)
    {
    case 0:
        option = 0;
        break;
    case 1:
        option = 1;
        break;
    }
    memloc = (void *)start_of_memory; //0
    if (option == 0)
    {
        for (int i = 0; i < SPLIT_SIZE; i++)
        {
            blockinfo[i] = malloc(sizeof(block));
            blockinfo[i]->start = 0;
            blockinfo[i]->end = 0;
            blockinfo[i]->size = 0;
            blockinfo[i]->next = NULL;
        }
        blockinfo[10]->start = 0;
        blockinfo[10]->end = 1024 * 1024 - 1; //1024*1024-1
        blockinfo[10]->size = 1024 * 1024;

        // allocated linked list
        allocated = malloc(sizeof(block));
        allocated->start = 0;
        allocated->end = 0;
        allocated->size = 0;
        allocated->next = NULL;
        allocated->address = NULL;
        memloc += 4;
    }
    else if (option == 1)
    {
        for (int i = 0; i < SPLIT_SIZE; i++)
        {
            blockinfoSLAB[i] = malloc(sizeof(block));
            blockinfoSLAB[i]->start = 0;
            blockinfoSLAB[i]->end = 0;
            blockinfoSLAB[i]->size = 0;
            blockinfoSLAB[i]->next = NULL;
        }
        blockinfoSLAB[10]->start = 0;
        blockinfoSLAB[10]->end = 1024 * 1024 - 1;
        blockinfoSLAB[10]->size = 1024 * 1024;

        // slab info linked list
        slabinfo = malloc(sizeof(slabinfos));
        slabinfo->type = 0;
        slabinfo->used = 0;
        slabinfo->start = 0;
        slabinfo->end = 0;
        slabinfo->size = 0;
        for (int i = 0; i < N_OBJS_PER_SLAB; i++)
        {
            slabinfo->arrayslab[i].address = NULL;
            slabinfo->arrayslab[i].start = 0;
            slabinfo->arrayslab[i].end = 0;
        }
        slabinfo->next = NULL;
        memloc += 8;
    }

    memsize = mem_size; //1MB
}

////////////////////////////////////////////////////////////////////////////
//
// Function     : my_malloc
// Description  : allocates memory segment using specified allocation algorithm
//
// Inputs       : size - size in bytes of the memory to be allocated
// Outputs      : -1 - if request cannot be made with the maximum mem_size requirement

void *my_malloc(int size)
{

    ////////////////////////////////////////////////
    ///////////////// BUDDY SYSTEM /////////////////
    ////////////////////////////////////////////////
    if (option == 0)
    {
        int extra_size = size + 4; //plus header size
        ////////////// ERROR CHECK //////////////
        if (extra_size < 0 || extra_size > memsize)
        {
            return (void *)-1;
        }

        int split_mem = memsize; //1 MB = 1024 * 1024 = 1048576
        int temp_size = 0;       // 넣을 애보다 더 작게 쪼개지지 않게
        int looping = 1;         // Initialize to 1

        int nearestval = findnearest2spower(extra_size); //1234 -> 2048
        while (looping)
        {
            for (int i = 0; i < SPLIT_SIZE + 1; i++)
            {
                if (i == 11)
                {
                    return (void *)-1;
                }
                else if (blockinfo[i]->size == 0)
                {
                    continue;
                }
                else if (blockinfo[i]->size < nearestval)
                {
                    continue; //skip
                }
                else
                {
                    // 우리가 필요한 사이즈 보다 더 크면 쪼개기
                    if (blockinfo[i]->size > nearestval)
                    {
                        ////////////////////////////////////////////////////
                        ////////////////// 윗칸에 뭐가 없을 때 /////////////////
                        ////////////////////////////////////////////////////
                        if (blockinfo[i - 1]->size == 0)
                        { //1mb: 0 ~ 1mb-1byte -> 0 ~ 524287 and 524288 ~ 1mb
                            // 쪼갠거 왼쪽
                            blockinfo[i - 1]->start = blockinfo[i]->start;
                            blockinfo[i - 1]->end = (blockinfo[i]->start + blockinfo[i]->end + 1) / 2 - 1;
                            blockinfo[i - 1]->size = blockinfo[i]->size / 2;
                            block *temp = malloc(sizeof(block));
                            // 쪼갠거 오른쪽
                            temp->start = (blockinfo[i]->start + blockinfo[i]->end) / 2 + 1;
                            temp->end = blockinfo[i]->end;
                            temp->size = blockinfo[i]->size / 2;
                            blockinfo[i - 1]->next = temp;
                            // 제일 처음 맨밑 1MB을 반으로 쪼개고 나서 0으로 초기화
                            if (blockinfo[i]->next == NULL)
                            {
                                blockinfo[i]->start = 0;
                                blockinfo[i]->end = 0;
                                blockinfo[i]->size = 0;
                            }
                            // 두개 있을때 하나를 없애고 맨뒤에걸 앞으로 옮기고 free 시킴
                            else if (blockinfo[i]->next->next == NULL)
                            {
                                block *tempempty = blockinfo[i]->next;
                                blockinfo[i]->start = blockinfo[i]->next->start;
                                blockinfo[i]->end = blockinfo[i]->next->end;
                                blockinfo[i]->size = blockinfo[i]->next->size;
                                blockinfo[i]->next = NULL;
                                free(tempempty);
                            }
                            //두개 넘게 있을때 두번째꺼를 앞에 옮기고, 앞에거를 3번째꺼로 next 시키고, 두번째걸 free 시킴
                            else
                            {
                                block *tempempty = blockinfo[i]->next;
                                blockinfo[i]->start = blockinfo[i]->next->start;
                                blockinfo[i]->end = blockinfo[i]->next->end;
                                blockinfo[i]->size = blockinfo[i]->next->size;
                                blockinfo[i]->next = NULL;
                                blockinfo[i]->next = blockinfo[i]->next->next;
                                free(tempempty);
                            }
                            temp_size = blockinfo[i - 1]->size; // split한것 중에서 가장 작은 것 저장
                            i = -1;                             // for loop 초기화
                        }
                        ////////////////////////////////////////////////////
                        ////////////////// 윗칸에 뭐가 있을때 //////////////////
                        ////////////////////////////////////////////////////
                        else if (blockinfo[i - 1]->size != 0)
                        {
                            block *temp1 = malloc(sizeof(block));
                            temp1->start = blockinfo[i]->start;
                            temp1->end = (blockinfo[i]->start + blockinfo[i]->end + 1) / 2 - 1;
                            temp1->size = blockinfo[i]->size / 2;
                            // 쪼갠거 오른쪽
                            block *temp2 = malloc(sizeof(block));
                            temp2->start = (blockinfo[i]->start + blockinfo[i]->end) / 2 + 1;
                            temp2->end = blockinfo[i]->end;
                            temp2->size = blockinfo[i]->size / 2;

                            block *tail = blockinfo[i - 1];
                            // tail로 Point 하게끔.
                            while (tail->next != NULL)
                            {
                                tail = tail->next;
                            }
                            if (tail->next == NULL)
                            {
                                tail->next = temp1;
                            }
                            temp1->next = temp2;

                            ////////////////////////////////////////////////////
                            ////////////////// 쪼갰던 원본 없애기 //////////////////
                            ////////////////////////////////////////////////////
                            // 하나있을때
                            if (blockinfo[i]->next == NULL)
                            {
                                blockinfo[i]->start = 0;
                                blockinfo[i]->end = 0;
                                blockinfo[i]->size = 0;
                            }
                            //두개있을때 하나를 없애고 맨뒤에걸 앞으로 옮기고 free 시킴
                            else if (blockinfo[i]->next->next == NULL)
                            {
                                block *tempempty = blockinfo[i]->next;
                                blockinfo[i]->start = blockinfo[i]->next->start;
                                blockinfo[i]->end = blockinfo[i]->next->end;
                                blockinfo[i]->size = blockinfo[i]->next->size;
                                blockinfo[i]->next = NULL;
                                free(tempempty);
                            }
                            //두개 이상 있을때 두번째꺼를 앞에 옮기고, 앞에거를 3번째꺼로 next 시키고, 두번째걸 free 시킴
                            else
                            {
                                if (blockinfo[i]->next != NULL)
                                {
                                    block *tempempty = blockinfo[i]->next;
                                    blockinfo[i]->start = blockinfo[i]->next->start;
                                    blockinfo[i]->end = blockinfo[i]->next->end;
                                    blockinfo[i]->size = blockinfo[i]->next->size;

                                    blockinfo[i]->next = blockinfo[i]->next->next;
                                    tempempty->next = NULL;
                                    free(tempempty);
                                }
                            }
                            temp_size = blockinfo[i - 1]->size;
                            i = -1;
                        }
                    }
                    else
                    {
                        looping = 0;
                        break;
                    }
                }
                // 더 쪼갤 필요가 있는지 체크
                // if(temp_size < extra_size) looping = 0;
            }
        } // while loop 끝

        int startinginfo = 0;

        // Allocate 한 애들 없애주기
        for (int j = 0; j < SPLIT_SIZE; j++)
        {
            startinginfo = blockinfo[j]->start;
            if (blockinfo[j]->size == 0)
            {
                continue;
            }
            else if (blockinfo[j]->size < nearestval)
            {
                continue; //skip
            }
            else
            {
                if (blockinfo[j]->next == NULL)
                {
                    saveAllocated(blockinfo[j], memloc, startinginfo);
                    blockinfo[j]->start = 0;
                    blockinfo[j]->end = 0;
                    blockinfo[j]->size = 0;
                }
                else if (blockinfo[j]->next->next == NULL)
                {
                    // 두개있을때 하나를 없애고 맨뒤에걸 앞으로 옮기고 free 시킴
                    //allocation 정보 update
                    saveAllocated(blockinfo[j], memloc, startinginfo);
                    block *tempempty = blockinfo[j]->next;
                    blockinfo[j]->start = blockinfo[j]->next->start;
                    blockinfo[j]->end = blockinfo[j]->next->end;
                    blockinfo[j]->size = blockinfo[j]->next->size;
                    blockinfo[j]->next = NULL;
                    free(tempempty);
                }
                else
                {
                    //두개 이상 있을때 두번째꺼를 앞에 옮기고, 앞에거를 3번째꺼로 next 시키고, 두번째걸 free 시킴
                    //allocation 정보 update
                    if (blockinfo[j]->next != NULL)
                    {
                        saveAllocated(blockinfo[j], memloc, startinginfo);
                        block *tempempty = blockinfo[j]->next;
                        blockinfo[j]->start = blockinfo[j]->next->start;
                        blockinfo[j]->end = blockinfo[j]->next->end;
                        blockinfo[j]->size = blockinfo[j]->next->size;

                        blockinfo[j]->next = blockinfo[j]->next->next;
                        tempempty->next = NULL;
                        free(tempempty);
                    }
                }
                sorting(allocated);
                temp_size = blockinfo[j]->size;
                break;
            }
        }

        // void *starting = memloc; // 처음엔 4
        // memloc += nearestval; //tempmem = memloc + starting_memory
        void *starting = memloc + startinginfo;
        return starting;
    }

    ////////////////////////////////////////////////
    ///////////////// SLAB ALLOC ///////////////////
    ////////////////////////////////////////////////
    else if (option == 1)
    {
        int extra_size = size + 4; //plus header size
        ////////////// ERROR CHECK //////////////
        if (extra_size < 0 || extra_size > memsize)
            return (void *)-1;

        int temp_size = 0; // 넣을 애보다 더 작게 쪼개지지 않게
        int looping = 1;   // Initialize to 1

        int size64 = extra_size * 64;
        int nearestval = findnearest2spower(size64); //1238*64= 79,232 -> 128kb
        int slabaddress = 0;

        slabinfos *check = slabinfo;
        while (check->next != NULL)
        {
            if (check->type != extra_size)
            {
                check = check->next;
            }
            else if (check->used == 64)
            {
                check = check->next;
            }
            else
            {
                break;
            }
        }
        // 1238 -> 4325 -> 23 -> null
        //처음에 들어올때 만약 slabinfo에 아무것도 없으면
        //맨 마지막의 slabinfo의 type이 extra size에 아무것도 없을 때
        //맨 마지막의 slabinfo의 used가 64 일때
        if (slabinfo->type == 0 || (check->next == NULL && check->type != extra_size) || (check->next == NULL && check->used == 64))
        {
            while (looping)
            {
                for (int i = 0; i < SPLIT_SIZE + 1; i++)
                {
                    if (i == 11)
                    {
                        return (void *)-1;
                    }
                    else if (blockinfoSLAB[i]->size == 0)
                    {
                        continue;
                    }
                    else if (blockinfoSLAB[i]->size < nearestval)
                    {
                        continue; //skip
                    }
                    else
                    {
                        // 우리가 필요한 사이즈 보다 더 크면 쪼개기
                        if (blockinfoSLAB[i]->size > nearestval)
                        {
                            ////////////////////////////////////////////////////
                            ////////////////// 윗칸에 뭐가 없을 때 /////////////////
                            ////////////////////////////////////////////////////
                            if (blockinfoSLAB[i - 1]->size == 0)
                            { //1mb: 0 ~ 1mb-1byte -> 0 ~ 524287 and 524288 ~ 1mb
                                // 쪼갠거 왼쪽
                                blockinfoSLAB[i - 1]->start = blockinfoSLAB[i]->start;
                                blockinfoSLAB[i - 1]->end = (blockinfoSLAB[i]->start + blockinfoSLAB[i]->end + 1) / 2 - 1;
                                blockinfoSLAB[i - 1]->size = blockinfoSLAB[i]->size / 2;
                                slab *temp = malloc(sizeof(slab));
                                // 쪼갠거 오른쪽
                                temp->start = (blockinfoSLAB[i]->start + blockinfoSLAB[i]->end) / 2 + 1;
                                temp->end = blockinfoSLAB[i]->end;
                                temp->size = blockinfoSLAB[i]->size / 2;
                                blockinfoSLAB[i - 1]->next = temp;
                                temp->next = NULL;
                                // 제일 처음 맨밑 1MB을 반으로 쪼개고 나서 0으로 초기화
                                if (blockinfoSLAB[i]->next == NULL)
                                {
                                    blockinfoSLAB[i]->start = 0;
                                    blockinfoSLAB[i]->end = 0;
                                    blockinfoSLAB[i]->size = 0;
                                    blockinfoSLAB[i]->next = NULL;
                                }
                                // 두개 있을때 하나를 없애고 맨뒤에걸 앞으로 옮기고 free 시킴
                                else if (blockinfoSLAB[i]->next->next == NULL)
                                {
                                    slab *tempempty = blockinfoSLAB[i]->next;
                                    blockinfoSLAB[i]->start = blockinfoSLAB[i]->next->start;
                                    blockinfoSLAB[i]->end = blockinfoSLAB[i]->next->end;
                                    blockinfoSLAB[i]->size = blockinfoSLAB[i]->next->size;
                                    blockinfoSLAB[i]->next = NULL;
                                    free(tempempty);
                                }
                                //두개 넘게 있을때 두번째꺼를 앞에 옮기고, 앞에거를 3번째꺼로 next 시키고, 두번째걸 free 시킴
                                else
                                {
                                    slab *tempempty = blockinfoSLAB[i]->next;
                                    blockinfoSLAB[i]->start = blockinfoSLAB[i]->next->start;
                                    blockinfoSLAB[i]->end = blockinfoSLAB[i]->next->end;
                                    blockinfoSLAB[i]->size = blockinfoSLAB[i]->next->size;
                                    blockinfoSLAB[i]->next = NULL;
                                    blockinfoSLAB[i]->next = blockinfoSLAB[i]->next->next;
                                    free(tempempty);
                                }
                                temp_size = blockinfoSLAB[i - 1]->size; // split한것 중에서 가장 작은 것 저장
                                i = -1;                                 // for loop 초기화
                            }
                            ////////////////////////////////////////////////////
                            ////////////////// 윗칸에 뭐가 있을때 //////////////////
                            ////////////////////////////////////////////////////
                            else if (blockinfoSLAB[i - 1]->size != 0)
                            {
                                slab *temp1 = malloc(sizeof(slab));
                                temp1->start = blockinfoSLAB[i]->start;
                                temp1->end = (blockinfoSLAB[i]->start + blockinfoSLAB[i]->end + 1) / 2 - 1;
                                temp1->size = blockinfoSLAB[i]->size / 2;
                                // 쪼갠거 오른쪽
                                slab *temp2 = malloc(sizeof(slab));
                                temp2->start = (blockinfoSLAB[i]->start + blockinfoSLAB[i]->end) / 2 + 1;
                                temp2->end = blockinfoSLAB[i]->end;
                                temp2->size = blockinfoSLAB[i]->size / 2;

                                slab *tail = blockinfoSLAB[i - 1];
                                // tail로 Point 하게끔.
                                while (tail->next != NULL)
                                {
                                    tail = tail->next;
                                }
                                if (tail->next == NULL)
                                {
                                    tail->next = temp1;
                                }
                                temp1->next = temp2;
                                temp2->next = NULL;

                                ////////////////////////////////////////////////////
                                ////////////////// 쪼갰던 원본 없애기 //////////////////
                                ////////////////////////////////////////////////////
                                // 하나있을때
                                if (blockinfoSLAB[i]->next == NULL)
                                {
                                    blockinfoSLAB[i]->start = 0;
                                    blockinfoSLAB[i]->end = 0;
                                    blockinfoSLAB[i]->size = 0;
                                    blockinfoSLAB[i]->next = NULL;
                                }
                                //두개있을때 하나를 없애고 맨뒤에걸 앞으로 옮기고 free 시킴
                                else if (blockinfoSLAB[i]->next->next == NULL)
                                {
                                    slab *tempempty = blockinfoSLAB[i]->next;
                                    blockinfoSLAB[i]->start = blockinfoSLAB[i]->next->start;
                                    blockinfoSLAB[i]->end = blockinfoSLAB[i]->next->end;
                                    blockinfoSLAB[i]->size = blockinfoSLAB[i]->next->size;
                                    blockinfoSLAB[i]->next = NULL;
                                    free(tempempty);
                                }
                                //두개 이상 있을때 두번째꺼를 앞에 옮기고, 앞에거를 3번째꺼로 next 시키고, 두번째걸 free 시킴
                                else
                                {
                                    if (blockinfoSLAB[i]->next != NULL)
                                    {
                                        slab *tempempty = blockinfoSLAB[i]->next;
                                        blockinfoSLAB[i]->start = blockinfoSLAB[i]->next->start;
                                        blockinfoSLAB[i]->end = blockinfoSLAB[i]->next->end;
                                        blockinfoSLAB[i]->size = blockinfoSLAB[i]->next->size;

                                        blockinfoSLAB[i]->next = blockinfoSLAB[i]->next->next;
                                        tempempty->next = NULL;
                                        free(tempempty);
                                    }
                                }
                                temp_size = blockinfoSLAB[i - 1]->size;
                                i = -1;
                            }
                        }
                        else
                        {
                            looping = 0;
                            break;
                        }
                    }
                    // 더 쪼갤 필요가 있는지 체크
                    // if(temp_size < extra_size) looping = 0;
                }
            } // while loop 끝

            int startinginfo = 0;
            //////////// Allocate 한 애들 없애주기 ////////////
            //////////// slabinfo로 가기//////////////
            for (int j = 0; j < SPLIT_SIZE; j++)
            {
                startinginfo = blockinfoSLAB[j]->start;
                if (blockinfoSLAB[j]->size == 0)
                {
                    continue;
                }
                else if (blockinfoSLAB[j]->size < nearestval)
                {
                    continue; //skip
                }
                else
                {
                    slab *tempnext;
                    if (blockinfoSLAB[j]->next != NULL)
                    {
                        tempnext = blockinfoSLAB[j]->next;
                    }
                    if (blockinfoSLAB[j]->next == NULL)
                    {
                        slabaddress = saveSLAB(blockinfoSLAB[j], extra_size, memloc, startinginfo);
                        blockinfoSLAB[j]->start = 0;
                        blockinfoSLAB[j]->end = 0;
                        blockinfoSLAB[j]->size = 0;
                        blockinfoSLAB[j]->next = NULL;
                    }
                    else if (blockinfoSLAB[j]->next != NULL && tempnext->next == NULL)
                    {
                        // 두개있을때 하나를 없애고 맨뒤에걸 앞으로 옮기고 free 시킴
                        //allocation 정보 update
                        slabaddress = saveSLAB(blockinfoSLAB[j], extra_size, memloc, startinginfo);
                        blockinfoSLAB[j]->start = tempnext->start; // 여기서 seg fault
                        blockinfoSLAB[j]->end = tempnext->end;
                        blockinfoSLAB[j]->size = tempnext->size;
                        blockinfoSLAB[j]->next = NULL;
                        free(tempnext);
                    }
                    else
                    {
                        //두개 이상 있을때 두번째꺼를 앞에 옮기고, 앞에거를 3번째꺼로 next 시키고, 두번째걸 free 시킴
                        //allocation 정보 update
                        if (blockinfoSLAB[j]->next != NULL)
                        {
                            slabaddress = saveSLAB(blockinfoSLAB[j], extra_size, memloc, startinginfo);
                            blockinfoSLAB[j]->start = tempnext->start;
                            blockinfoSLAB[j]->end = tempnext->end;
                            blockinfoSLAB[j]->size = tempnext->size;
                            blockinfoSLAB[j]->next = tempnext->next;
                            tempnext->next = NULL;
                            free(tempnext);
                        }
                    }
                    // sorting(allocated);
                    temp_size = blockinfoSLAB[j]->size;
                    break;
                }
            }
        }
        //처음에 들어올때 만약 slabinfo에 누군가가 있으면
        else if (slabinfo->type != 0)
        {
            slabinfos *tempslabinfo = slabinfo;
            int j = 0;
            while (tempslabinfo != NULL)
            {

                if (tempslabinfo->type != extra_size || tempslabinfo->used == 64)
                {
                    tempslabinfo = tempslabinfo->next;
                }
                else if (tempslabinfo->type == extra_size)
                {
                    if (tempslabinfo->arrayslab[j].address == NULL && (j < 64))
                    {
                        int startint = tempslabinfo->start;
                        (tempslabinfo->arrayslab[j]).start = startint + (extra_size * j); // 여기서 blockinfoslab next 값들이 바꾸
                        (tempslabinfo->arrayslab[j]).end = startint + (extra_size + (extra_size * j) - 1);
                        tempslabinfo->arrayslab[j].address = memloc + tempslabinfo->arrayslab[j].start; //need this for my_free later
                        slabaddress = tempslabinfo->arrayslab[j].start;
                        tempslabinfo->used += 1;
                        break;
                    }
                    else if (j < 64)
                    {
                        j++;
                    }
                    else if (j == 64)
                    {
                        //새로운 slab만들어주기
                    }
                }
            }
        }

        void *starting = memloc + slabaddress;
        return starting;
    }
}

////////////////////////////////////////////////////////////////////////////
//
// Function     : my_free
// Description  : deallocated the memory segment being passed by the pointer
//
// Inputs       : ptr - pointer to the memory segment to be free'd
// Outputs      :

void my_free(void *ptr)
{

    if (option == 0)
    {
        // Free, 합쳐지는거
        block *curr = allocated;
        block *prev = allocated;
        block *tempMalloc = malloc(sizeof(block));
        // allocated 맨앞에꺼만 비교
        if (curr->address == ptr)
        {
            // blockinfo 에 집어넣을거 세이브
            tempMalloc->start = curr->start;
            tempMalloc->end = curr->end;
            tempMalloc->size = curr->size;
            tempMalloc->address = NULL;
            tempMalloc->next = NULL;

            ///////////// allocated 맨앞애 지우기 /////////////
            // allocated 에 하나만 있을때
            if (curr->next == NULL)
            {
                allocated->start = 0;
                allocated->end = 0;
                allocated->size = 0;
                allocated->address = NULL;
            }
            // allocated 에 여러가 있을때
            else if (curr->next != NULL)
            {
                allocated = allocated->next;
                curr->next = NULL;
                free(curr);
            }
        }
        // 맨앞에꺼가 Ptr 이랑 다를때
        else
        {
            curr = curr->next;
            while (curr->next != NULL)
            {
                if (curr->address == ptr)
                {
                    tempMalloc->start = curr->start;
                    tempMalloc->end = curr->end;
                    tempMalloc->size = curr->size;
                    tempMalloc->address = NULL;
                    tempMalloc->next = NULL;

                    // allocated에 하나만 있을 때
                    if (allocated->next == NULL)
                    {
                        allocated->start = 0;
                        allocated->end = 0;
                        allocated->size = 0;
                        allocated->address = NULL;
                    }
                    //앞에 만 그리고, allocated 하나 이상
                    else if (allocated->next != NULL && curr == ptr)
                    {
                        allocated = allocated->next;
                        curr->next = NULL;
                        free(curr);
                    }
                    //중간
                    else if (allocated->next != NULL && curr == ptr)
                    {
                        prev->next = curr->next;
                        curr->next = NULL;
                        free(curr);
                    }
                    //마지막
                    else if (allocated->next != NULL && curr->next != NULL)
                    {
                        prev->next = NULL;
                        free(curr);
                    }
                    break;
                }
                else
                {
                    curr = curr->next;
                    prev = prev->next;
                }
            }
            // 마지막까지 가지 못해서 애만 스킵합
            if (curr->next == NULL)
            {
                tempMalloc->start = curr->start;
                tempMalloc->end = curr->end;
                tempMalloc->size = curr->size;
                tempMalloc->address = NULL;
                tempMalloc->next = NULL;
                prev->next = NULL;
                free(curr);
            }
        }
        //////////////////////////////////////////////////
        //////////////// blockinfo 에 넣기 /////////////////
        //////////////////////////////////////////////////
        int tempsize = 1024;
        int combinedSize = 0;
        for (int i = 0; i < SPLIT_SIZE; i++)
        {
            if (tempMalloc->size == tempsize)
            {
                block *tail = blockinfo[i];
                // blockinfo 칸에 아무도 없을때
                if (blockinfo[i]->size == 0)
                {
                    blockinfo[i]->start = tempMalloc->start;
                    blockinfo[i]->end = tempMalloc->end;
                    blockinfo[i]->size = tempMalloc->size;
                    blockinfo[i]->address = NULL;
                    blockinfo[i]->next = NULL;
                    free(tempMalloc);
                }
                // blcokinfo 칸에 누가 있을때
                else
                {
                    while (tail->next != NULL)
                    {
                        tail = tail->next;
                    }
                    tail->next = tempMalloc;
                }
                //sort
                sorting(blockinfo[i]);
            }
            // 현재 Tempsize 랑 다르면 다음 사이즈로
            else
            {
                tempsize = tempsize * 2;
                continue;
            }
            break; // 끝
        }
        //////////////////////////////////////////////////
        //////////////// blockinfo 를 합치기 ///////////////
        //////////////////////////////////////////////////

        for (int i = 0; i < SPLIT_SIZE; i++)
        {
            // 두개 있을때
            if (blockinfo[i]->next != NULL)
            {
                block *prev = blockinfo[i];
                block *curr = blockinfo[i];
                block *next = blockinfo[i]->next;

                // 둘이 붙어있고 && 둘의 합이 합친 사이즈면 합쳐주기
                if ((next->start - curr->end == 1) && ((next->end - curr->start + 1) % curr->size * 2 == 0))
                {
                    block *temp = malloc(sizeof(block));
                    temp->start = curr->start;
                    temp->end = next->end;
                    temp->size = curr->size * 2;
                    temp->address = NULL;
                    temp->next = NULL;
                    block *tail = blockinfo[i + 1];
                    // 합쳐주기 (다음칸으로 보내기)
                    while (tail->next != NULL)
                    {
                        tail = tail->next;
                    }
                    if (tail->size == 0)
                    {
                        tail->start = temp->start;
                        tail->end = temp->end;
                        tail->size = temp->size;
                        tail->address = NULL;
                        tail->next = NULL;
                        free(temp);
                    }
                    else
                    {
                        tail->next = temp;
                        sorting(blockinfo[i + 1]);
                    }

                    ///////////// 다음칸으로 옮기고 나서 원래있던 합친 둘을 없애기 /////////////
                    // 노드가 두개밖에 없을때 둘다 없애기

                    if (next->next == NULL && curr == blockinfo[i])
                    {
                        curr->start = 0;
                        curr->end = 0;
                        curr->size = 0;
                        curr->address = NULL;
                        curr->next == NULL;
                        free(next);
                    }
                    // 노드가 세개 이상일때 두개 없애고 그다음껄 맨앞으로
                    else if (next->next != NULL && curr == blockinfo[i])
                    {
                        blockinfo[i] = next->next;
                        curr->next = NULL;
                        next->next = NULL;
                        free(curr);
                        free(next);
                    }
                    //line 536 - 549 - 아직 확인 안함
                    // 아직 컨디션 부족
                    //노드가 세개 이상일때 중간 두개 없애고 그다음껄 맨앞에꺼 뒤로 5개 중에 1 2-prev 3-curr 4-next 5 <- 이 중에서 3-curr를 없앤다.
                    else if (curr != blockinfo[i] && next->next != NULL)
                    {
                        curr->next = NULL;
                        prev->next = next;
                        free(curr);
                    }
                    //노드가 전부 뒤에 있을때, 뒤에 두개를 없앤다.
                    else if (next->next == NULL)
                    {
                        curr->next = NULL;
                        prev->next = NULL;
                        free(curr);
                        free(next);
                    }
                    sorting(blockinfo[i]);
                    i = -1;
                }
                // 둘이 버디가 아닐때 (떨어져있을때)
                // prev, curr, next 다음 노드로 옮긴다.
                // prev = curr
                else
                {
                    if (prev = curr)
                    {
                        curr = curr->next;
                        if (next->next != NULL)
                        {
                            next = next->next;
                        }
                    }
                    else
                    {
                        prev = prev->next;
                        curr = curr->next;
                        if (next->next != NULL)
                        {
                            next = next->next;
                        }
                    }
                }
            }
        }
    }

    if (option == 1)
    {
        slabinfos *location = slabinfo; //curr
        slabinfos *prev = slabinfo;
        slab *toBlockMalloc = malloc(sizeof(slab)); //blockinfoSLAB으로 들어갈 block
        // location->arrayslab[?]->address == ptr -> location->arrayslab[?]->address = NULL and used -= 1
        int i = 0;
        while (location != NULL)
        {
            if (i == 64)
            {
                if (location->next != NULL)
                {
                    if (location == prev)
                    {
                        //do nothing
                    }
                    else
                    {
                        prev = prev->next;
                    }
                    location = location->next;

                    i = 0;
                }
                else
                {
                    break;
                }
            }
            else if (location->arrayslab[i].address != ptr)
            {
                i++;
            }
            // ptr 이랑 매치할때
            else
            {
                location->arrayslab[i].address = 0;
                location->arrayslab[i].start = 0;
                location->arrayslab[i].end = 0;
                location->used -= 1;
                break;
            }
        }
        if (location->used != 0)
        {
            //아무것도 안하기
        }
        // if used == 0 일때 slabinfos가 사라지고, blockinfoSLAB의 양을 추가
        else
        {
            //remove block from the slab / insert the block in blockinfoSLAB
            while (location->next != NULL)
            {
                toBlockMalloc->start = location->start;
                toBlockMalloc->end = location->end;
                toBlockMalloc->size = location->size;
                toBlockMalloc->next = NULL;

                // slab 이 하나밖에 없을때
                if (slabinfo->next == NULL && location == slabinfo)
                {
                    slabinfo->start = 0;
                    slabinfo->end = 0;
                    slabinfo->size = 0;
                    slabinfo->type = 0;
                    slabinfo->used = 0;
                    for (int i = 0; i < 64; i++)
                    {
                        slabinfo->arrayslab[i].address = NULL;
                        slabinfo->arrayslab[i].start = 0;
                        slabinfo->arrayslab[i].end = 0;
                    }
                }
                // location이 첫번째자리에 있는데 slab 이 여러개 있을때
                else if (location->next != NULL && location == slabinfo)
                {
                    slabinfo = slabinfo->next;
                    free(location);
                }
                // loaction(curr)이 두번째 자리에 있을때/ slab 이 여러개 있을때
                else if (location->next == NULL && location != slabinfo)
                {
                    prev->next = NULL;
                    free(location);
                }
                // location이 세번째 자리에 있을때 / slab 이 여러개 있을때
                else if (location->next != NULL && location != slabinfo)
                {
                    prev->next = location->next;
                    location->next = NULL;
                    free(location);
                }
                else
                {
                    location = location->next;
                    if (location == prev)
                    {
                        prev = prev->next;
                    }
                    continue;
                }
                break;
            }

            //////////////////////////////////////////////////
            ////////////// blockinfoSLAB 에 넣기 ///////////////
            //////////////////////////////////////////////////
            int tempsize = 1024;
            int combinedSize = 0;

            for (int i = 0; i < SPLIT_SIZE; i++)
            {
                if (toBlockMalloc->size == tempsize)
                {
                    slab *tail = blockinfoSLAB[i];
                    // blockinfo 칸에 아무도 없을때
                    if (blockinfoSLAB[i]->size == 0)
                    {
                        blockinfoSLAB[i]->start = toBlockMalloc->start;
                        blockinfoSLAB[i]->end = toBlockMalloc->end;
                        blockinfoSLAB[i]->size = toBlockMalloc->size;
                        blockinfoSLAB[i]->next = NULL;
                        free(toBlockMalloc);
                    }
                    // blcokinfo 칸에 누가 있을때
                    else
                    {
                        while (tail->next != NULL)
                        {
                            tail = tail->next;
                        }
                        tail->next = toBlockMalloc;
                    }
                    //sort
                    sorting2(blockinfoSLAB[i]);
                }
                // 현재 Tempsize 랑 다르면 다음 사이즈로
                else
                {
                    tempsize = tempsize * 2;
                    continue;
                }
                break; // 끝
            }
            //////////////////////////////////////////////////
            ///////// blockinfoSLAB 같은 사이즈 합치기 ///////////
            //////////////////////////////////////////////////
            for (int i = 0; i < SPLIT_SIZE; i++)
            {
                // 두개 있을때
                if (blockinfoSLAB[i]->next != NULL)
                {
                    slab *prev = blockinfoSLAB[i];
                    slab *curr = blockinfoSLAB[i];
                    slab *next = blockinfoSLAB[i]->next;

                    // 둘이 붙어있고 && 둘의 합이 합친 사이즈면 합쳐주기
                    if ((next->start - curr->end == 1) && ((next->end - curr->start + 1) % curr->size * 2 == 0))
                    {
                        slab *temp = malloc(sizeof(slab));
                        temp->start = curr->start;
                        temp->end = next->end;
                        temp->size = curr->size * 2;
                        temp->next = NULL;
                        slab *tail = blockinfoSLAB[i + 1];
                        // 합쳐주기 (다음칸으로 보내기)
                        while (tail->next != NULL)
                        {
                            tail = tail->next;
                        }
                        if (tail->size == 0)
                        {
                            tail->start = temp->start;
                            tail->end = temp->end;
                            tail->size = temp->size;
                            tail->next = NULL;
                            free(temp);
                        }
                        else
                        {
                            tail->next = temp;
                            sorting2(blockinfoSLAB[i + 1]);
                        }

                        ///////////// 다음칸으로 옮기고 나서 원래있던 합친 둘을 없애기 /////////////
                        // 노드가 두개밖에 없을때 둘다 없애기

                        if (next->next == NULL && curr == blockinfoSLAB[i])
                        {
                            curr->start = 0;
                            curr->end = 0;
                            curr->size = 0;
                            curr->next == NULL;
                            free(next);
                        }
                        // 노드가 세개 이상일때 두개 없애고 그다음껄 맨앞으로
                        else if (next->next != NULL && curr == blockinfoSLAB[i])
                        {
                            blockinfoSLAB[i] = next->next;
                            curr->next = NULL;
                            next->next = NULL;
                            free(curr);
                            free(next);
                        }
                        //line 536 - 549 - 아직 확인 안함
                        // 아직 컨디션 부족
                        //노드가 세개 이상일때 중간 두개 없애고 그다음껄 맨앞에꺼 뒤로 5개 중에 1 2-prev 3-curr 4-next 5 <- 이 중에서 3-curr를 없앤다.
                        else if (curr != blockinfoSLAB[i] && next->next != NULL)
                        {
                            curr->next = NULL;
                            prev->next = next;
                            free(curr);
                        }
                        //노드가 전부 뒤에 있을때, 뒤에 두개를 없앤다.
                        else if (next->next == NULL)
                        {
                            curr->next = NULL;
                            prev->next = NULL;
                            free(curr);
                            free(next);
                        }
                        sorting2(blockinfoSLAB[i]);
                        i = -1;
                    }
                    // 둘이 버디가 아닐때 (떨어져있을때)
                    // prev, curr, next 다음 노드로 옮긴다.
                    // prev = curr
                    else
                    {
                        if (prev = curr)
                        {
                            curr = curr->next;
                            if (next->next != NULL)
                            {
                                next = next->next;
                            }
                        }
                        else
                        {
                            prev = prev->next;
                            curr = curr->next;
                            if (next->next != NULL)
                            {
                                next = next->next;
                            }
                        }
                    }
                }
            }
        }
    }
}