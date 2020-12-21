
/**
 * File             : 473_mm.c
 * Description      : Project 3: Virtual Memory - Paging
 * Author(s)        : Sung Woo Oh, Jihoon (Jeremiah) Park
*/

#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <signal.h>
#include <string.h>
#include "473_mm.h"

//linked list 만들기
/*

policy 1 - FIFO 
linked list를 만듬. Initially 4개.
그 후에 모두가 가득차면... 
하나를 malloc, header를 다음으로 옮김.
끝.

policy 2 - third chance 
circular linked list를 4개나 만듬. 
FIFO와 비슷한 theory 그러나 2번 봐주면 버리고 
새로운 애를 집어 넣음

*/

void *g_vm;
int g_vm_size;
int g_n_frames;
int g_page_size;
int g_policy;
unsigned int start_of_vm = 0;

typedef struct init_FIFO
{
    void *address;  //siginfo_t si->si_addr
    int vpagenum;   //0-15
    int type;       // errbit / fault type
    int permission; //NONE READ WRITE READ/WRITE = 0 1 2 3
    int pframenum;  //physical frame num 0-3
    int offset;     //last 12 bits
    int evicted;
    struct init_FIFO *next;
} init_FIFO;

typedef struct init_Third
{
    void *address;  //siginfo_t si->si_addrprintf("errbit is not 2\n");
    int vpagenum;   //0-15
    int type;       // errbit / fault type
    int permission; //NONE READ WRITE READ/WRITE = 0 1 2 3
    int pframenum;  //physical frame num 0-3
    int offset;     //last 12 bits
    int evicted;
    int ref;
    int modified;
    int chance;
    struct init_Third *next;
} init_Third;

typedef struct evict_info
{
    int vpagenum;   //0-15
    int type;       // errbit / fault type
    int permission; //NONE READ WRITE READ/WRITE = 0 1 2 3
    int pframenum;  //physical frame num 0-3
    int offset;     //last 12 bits
    int evicted;
    int modified; //third chance replacement ( FIFO x )
} evict_info;

init_FIFO *FIFO;
init_FIFO *head;
evict_info *evict;
init_Third *Third;
init_Third *head3;

void signal_handler(int num, siginfo_t *si, ucontext_t *ctx)
{

    greg_t errbit = ctx->uc_mcontext.gregs[REG_ERR] & 0x2; //read -> 0x100 & 0x010 = 0 //write -> 0x110 & 0x010 = 2
    // greg_t errbit2 = ctx->uc_mcontext.gregs[REG_ERR]; //4 == 0x100 6 == 0x110 2=0x010
    long int tempoffset = (si->si_addr) - (g_vm);
    long int offset = tempoffset & 0xFFF;
    int virtual_page_num = (tempoffset & 0xF000) / (4096);
    int write_back = 0;
    int swap = 1;

    if (g_policy == 1)
    {
        if (errbit == 0 || errbit == 2)
        {
            init_FIFO *checking = FIFO;
            while (checking->next != NULL)
            {
                if (checking->vpagenum == virtual_page_num)
                {
                    swap = 0; //not swapping
                    break;
                }
                else
                {
                    checking = checking->next;
                }
            }

            // vpagenum 이 똑같은 애면 안바꿔줘도 됨
            if (checking->vpagenum == virtual_page_num)
            {
                swap = 0;
            }

            if (swap == 1)
            { // swap이 1이면 바꿔줘야함
                // 바꿔줄애 (뺄 애 Evict에 저장)
                if (head->vpagenum != -1 || head->vpagenum != virtual_page_num)
                {
                    evict->vpagenum = head->vpagenum;
                    if (head->vpagenum != -1)
                    {
                        head->evicted = 1;
                    }
                }

                //evict한 애가 write이나 read/write면은 write_back = 1
                if ((head->type == 1) || (head->type == 2))
                { //2: write, 3: read/write
                    write_back = 1;
                }
                // 새로 들어온애를 head에 저장
                head->address = si->si_addr;
                head->vpagenum = virtual_page_num;
                head->type = -1;
                head->offset = offset;
                // head 다음으로 옮겨주기

                // check err is from read or write
                if (errbit == 0)
                {
                    mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ);
                    head->permission = PROT_READ;
                    head->type = 0;
                }

                else if (errbit == 2)
                {
                    mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE);
                    head->permission = PROT_WRITE;
                    head->type = 1;
                }
                else
                {
                    //skip
                }

                mm_logger(virtual_page_num, head->type, evict->vpagenum, write_back, offset | (head->pframenum << 12)); //type fault 고쳐주지
                if (head->evicted == 1)
                {
                    // printf("got in\n");
                    mprotect(g_vm + (g_page_size * evict->vpagenum), g_page_size, PROT_NONE);
                }
                (head->next == NULL) ? (head = FIFO) : (head = head->next);
            }
            if (swap == 0)
            {
                init_FIFO *temp0 = FIFO;
                while (temp0->next != NULL)
                {
                    if (temp0->vpagenum == virtual_page_num)
                    {
                        if (errbit == 0)
                        {
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                            temp0->permission = PROT_READ | PROT_WRITE;
                            temp0->type = 2;
                        }

                        else if (errbit == 2)
                        {
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                            temp0->permission = PROT_WRITE | PROT_READ;
                            temp0->type = 2;
                        }
                        else
                        {
                            // skip
                        }
                        mm_logger(virtual_page_num, temp0->type, -1, write_back, offset | (temp0->pframenum << 12));
                        break;
                    }
                    else
                    {
                        temp0 = temp0->next;
                    }
                }
                if ((temp0->vpagenum = virtual_page_num) && (temp0->next == NULL))
                {
                    if (errbit == 0)
                    {
                        mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                        temp0->permission = PROT_READ | PROT_WRITE;
                        temp0->type = 2;
                    }

                    else if (errbit == 2)
                    {
                        mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                        temp0->permission = PROT_WRITE | PROT_READ;
                        temp0->type = 2;
                    }
                    else
                    {
                        // skip
                    }
                    mm_logger(virtual_page_num, temp0->type, -1, write_back, offset | (temp0->pframenum << 12));
                }
            }
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////// Third Chance /////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    if (g_policy == 2)
    {
        if (errbit == 0 || errbit == 2)
        {
            init_Third *checking = Third;
            while (checking->next != NULL)
            {
                if (checking->vpagenum == virtual_page_num)
                {
                    swap = 0; //not swapping
                    break;
                }
                else
                {
                    checking = checking->next;
                }
            }
            // vpagenum 이 똑같은 애면 안바꿔줘도 됨
            if (checking->vpagenum == virtual_page_num)
            {
                swap = 0;
            }

            if (swap == 1)
            { // swap이 1이면 바꿔줘야함
                // 바꿔줄애 (뺄 애 Evict에 저장)
                while (1)
                {
                    if (head3->ref == 0 && head3->modified == 0)
                    {
                        if (head3->vpagenum != -1 || head3->vpagenum != virtual_page_num)
                        {
                            evict->vpagenum = head3->vpagenum;
                            evict->modified = head3->modified;
                            if (head3->vpagenum != -1)
                            {
                                head3->evicted = 1;
                            }
                        }

                        //evict한 애가 write이나 read/write면은 write_back = 1
                        if (head3->permission == 2 || head3->permission == 3)
                        { //2: write, 3: read/write
                            write_back = 1;
                        }

                        if (head3->vpagenum >= 0)
                        {
                            mprotect(g_vm + (g_page_size * head3->vpagenum), g_page_size, PROT_NONE);
                            head3->permission = 0;
                        }

                        // 새로 들어온애를 head에 저장
                        head3->address = si->si_addr;
                        head3->vpagenum = virtual_page_num;
                        head3->type = -1;
                        head3->offset = offset;
                        head3->ref = 1;
                        head3->modified = 0;
                        head3->chance = 0;
                        // head3 다음으로 옮겨주기

                        // check err is from read or write
                        if (errbit == 0)
                        {
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ);
                            head3->permission = PROT_READ;
                            head3->type = 0;
                        }
                        else if (errbit == 2)
                        {
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE);
                            head3->permission = PROT_WRITE;
                            head3->type = 1;
                            head3->modified = 1;
                        }
                        else
                        {
                            // skip
                        }
                        mm_logger(virtual_page_num, head3->type, evict->vpagenum, write_back, offset | (head3->pframenum << 12));

                        (head3->next == NULL) ? (head3 = Third) : (head3 = head3->next);
                        break;
                    }
                    else if (head3->ref == 1 && head3->modified == 0)
                    {
                        head3->ref = 0;
                        head3->chance += 1;
                        if (head3->vpagenum >= 0)
                        {
                            mprotect(g_vm + (g_page_size * head3->vpagenum), g_page_size, PROT_NONE);
                            // head3->permission = 0;
                        }
                        (head3->next == NULL) ? (head3 = Third) : (head3 = head3->next);
                    }
                    else if (head3->modified == 1)
                    {
                        if (head3->chance < 2)
                        {
                            head3->chance += 1;
                            head3->ref = 0;
                            if (head3->vpagenum >= 0)
                            {
                                mprotect(g_vm + (g_page_size * head3->vpagenum), g_page_size, PROT_NONE);
                                // head3->permission = 0;
                            }
                            (head3->next == NULL) ? (head3 = Third) : (head3 = head3->next);
                        }
                        else if (head3->chance == 2)
                        {
                            if (head3->vpagenum != -1 || head3->vpagenum != virtual_page_num)
                            {
                                evict->vpagenum = head3->vpagenum;
                                evict->modified = head3->modified;
                                if (head3->vpagenum != -1)
                                {
                                    head3->evicted = 1;
                                }
                            }

                            //evict한 애가 write이나 read/write면은 write_back = 1
                            if (head3->type != 0 && head3->type != -1)
                            { //2: write, 3: read/write
                                write_back = 1;
                            }

                            if (head3->vpagenum >= 0)
                            {
                                mprotect(g_vm + (g_page_size * head3->vpagenum), g_page_size, PROT_NONE);
                                head3->permission = 0;
                            }
                            // 새로 들어온애를 head에 저장
                            head3->address = si->si_addr;
                            head3->vpagenum = virtual_page_num;
                            head3->type = -1;
                            head3->offset = offset;
                            head3->ref = 1;
                            head3->modified = 0;
                            head3->chance = 0;
                            // head3 다음으로 옮겨주기

                            // check err is from read or write
                            if (errbit == 0)
                            {
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ);
                                head3->permission = PROT_READ;
                                head3->type = 0;
                            }
                            else if (errbit == 2)
                            {
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE);
                                head3->permission = PROT_WRITE;
                                head3->type = 1;
                                head3->permission = 1;
                            }
                            else
                            {
                                // skip
                            }
                            mm_logger(virtual_page_num, head3->type, evict->vpagenum, write_back, offset | (head3->pframenum << 12));

                            (head3->next == NULL) ? (head3 = Third) : (head3 = head3->next);
                            break;
                        }
                    }
                }
            }
            if (swap == 0)
            {
                init_Third *temp0 = Third;
                while (temp0->next != NULL)
                {
                    if (temp0->vpagenum == virtual_page_num)
                    {
                        if (errbit == 0)
                        {
                            if (temp0->type == 0)
                            { //read <- read
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ);
                                temp0->permission = PROT_READ;
                                temp0->type = 3;
                            }
                            else if (temp0->type == 1)
                            { //write <- read
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                                temp0->permission = PROT_READ | PROT_WRITE;
                                temp0->type = 3;
                                // temp0->modified = 1;
                            }
                            else if (temp0->type == 2)
                            { //read + write <- read
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ | PROT_WRITE);
                                temp0->permission = PROT_READ | PROT_WRITE;
                                temp0->type = 3;
                            }
                        }
                        else if (errbit == 2)
                        {
                            if (temp0->type == 0)
                            { //read <- write
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                                temp0->permission = PROT_WRITE | PROT_READ;
                                temp0->type = 2;
                                temp0->modified = 1;
                            }
                            else if (temp0->type == 1)
                            { //write <- write
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE);
                                temp0->permission = PROT_WRITE;
                                temp0->type = 4;
                                temp0->modified = 1;
                            }
                            else if (temp0->type == 2)
                            { //read/write <- write
                                mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                                temp0->permission = PROT_WRITE | PROT_READ;
                                temp0->type = 4;
                                temp0->modified = 1;
                            }
                        }
                        else
                        {
                            // skip
                        }
                        mm_logger(virtual_page_num, temp0->type, -1, write_back, offset | (temp0->pframenum << 12));
                        break;
                    }
                    else
                    {
                        temp0 = temp0->next;
                    }
                }
                if ((temp0->vpagenum = virtual_page_num) && (temp0->next == NULL))
                {
                    if (errbit == 0)
                    {
                        if (temp0->type == 0)
                        { //read <- read
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ);
                            temp0->type = 3;
                            temp0->permission = 1;
                        }
                        else if (temp0->type == 1)
                        { //write <- read
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                            temp0->permission = PROT_READ | PROT_WRITE;
                            temp0->type = 3;
                            // temp0->modified = 1;
                        }
                        else if (temp0->type == 2)
                        { //read + write <- read
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_READ | PROT_WRITE);
                            temp0->type = 3;
                            temp0->permission = PROT_READ | PROT_WRITE;
                        }
                    }
                    else if (errbit == 2)
                    {
                        if (temp0->type == 0)
                        { //read <- write
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                            temp0->permission = PROT_WRITE | PROT_READ;
                            temp0->type = 2;
                            temp0->modified = 1;
                        }
                        else if (temp0->type == 1)
                        { //write <- write
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE);
                            temp0->type = 4;
                            temp0->permission = PROT_WRITE;
                            temp0->modified = 1;
                        }
                        else if (temp0->type == 2)
                        { //read/write <- write
                            mprotect(g_vm + (g_page_size * virtual_page_num), g_page_size, PROT_WRITE | PROT_READ);
                            temp0->type = 4;
                            temp0->permission = PROT_READ | PROT_WRITE;
                            temp0->modified = 1;
                        }
                    }
                    else
                    {
                        // skip
                    }
                    mm_logger(virtual_page_num, temp0->type, -1, write_back, offset | (temp0->pframenum << 12));
                }
                temp0->chance = 0;
                temp0->ref = 1;
            }
        }
    }
    return;
}
//none = 0, read = 1, write = 2,
// Prot_none = 0, Prot_read = 1, prot_w
//fault type
// 0: PROT_NONE -> PROT_READ
// 1: PROT_NONE -> PROT_WRITE
// 2: PROT_READ -> PROT_READ | PROT_WRITE

void mm_init(void *vm, int vm_size, int n_frames, int page_size, int policy)
{

    g_vm = vm;
    g_vm_size = vm_size;
    g_n_frames = n_frames;
    g_page_size = page_size;
    g_policy = policy;

    if (policy == 1)
    {
        /////////////// Initialization ///////////////
        FIFO = malloc(sizeof(init_FIFO));
        FIFO->vpagenum = -1; //-1 없다는 의미
        FIFO->type = -1;     //errbit
        FIFO->permission = PROT_NONE;
        FIFO->pframenum = 0;
        FIFO->offset = -1;
        FIFO->evicted = 0;

        init_FIFO *FIFO2 = malloc(sizeof(init_FIFO));
        FIFO2->vpagenum = -1;
        FIFO2->type = -1;
        FIFO2->permission = PROT_NONE;
        FIFO2->pframenum = 1;
        FIFO2->offset = -1;
        FIFO->evicted = 0;

        init_FIFO *FIFO3 = malloc(sizeof(init_FIFO));
        FIFO3->vpagenum = -1;
        FIFO3->type = -1;
        FIFO3->permission = PROT_NONE;
        FIFO3->pframenum = 2;
        FIFO3->offset = -1;
        FIFO->evicted = 0;

        init_FIFO *FIFO4 = malloc(sizeof(init_FIFO));
        FIFO4->vpagenum = -1;
        FIFO4->type = -1;
        FIFO4->permission = PROT_NONE;
        FIFO4->pframenum = 3;
        FIFO4->offset = -1;
        FIFO->evicted = 0;

        FIFO->next = FIFO2;
        FIFO2->next = FIFO3;
        FIFO3->next = FIFO4;
        FIFO4->next = NULL;

        head = FIFO;

        evict = malloc(sizeof(evict_info));
        evict->vpagenum = -1;
        evict->type = -1;
        evict->permission = -1;
        evict->pframenum = -1;
        evict->offset = -1;
        evict->evicted = 0;
        evict->modified = -1;
    }

    if (policy == 2)
    {
        Third = malloc(sizeof(init_Third));
        Third->vpagenum = -1; //-1 없다는 의미
        Third->type = -1;     //errbit
        Third->permission = PROT_NONE;
        Third->pframenum = 0;
        Third->offset = -1;
        Third->evicted = 0;
        Third->ref = 0;
        Third->modified = 0;
        Third->chance = 0;

        init_Third *Third2 = malloc(sizeof(init_Third));
        Third2->vpagenum = -1;
        Third2->type = -1;
        Third2->permission = PROT_NONE;
        Third2->pframenum = 1;
        Third2->offset = -1;
        Third2->evicted = 0;
        Third2->ref = 0;
        Third2->modified = 0;
        Third2->chance = 0;

        init_Third *Third3 = malloc(sizeof(init_Third));
        Third3->vpagenum = -1;
        Third3->type = -1;
        Third3->permission = PROT_NONE;
        Third3->pframenum = 2;
        Third3->offset = -1;
        Third3->evicted = 0;
        Third3->ref = 0;
        Third3->modified = 0;
        Third3->chance = 0;

        init_Third *Third4 = malloc(sizeof(init_Third));
        Third4->vpagenum = -1;
        Third4->type = -1;
        Third4->permission = PROT_NONE;
        Third4->pframenum = 3;
        Third4->offset = -1;
        Third4->evicted = 0;
        Third4->ref = 0;
        Third4->modified = 0;
        Third4->chance = 0;

        Third->next = Third2;
        Third2->next = Third3;
        Third3->next = Third4;
        Third4->next = NULL;

        head3 = Third;

        evict = malloc(sizeof(evict_info));
        evict->vpagenum = -1;
        evict->type = -1;
        evict->permission = -1;
        evict->pframenum = -1;
        evict->offset = -1;
        evict->evicted = 0;
        evict->modified = -1;
    }

    // PROT_NONE
    for (int i = 0; i < 16; i++)
    {
        mprotect(g_vm + page_size * i, page_size, PROT_NONE);
    }

    //////////////// sigaction ////////////////
    struct sigaction s, u;
    s.sa_flags = SA_SIGINFO;
    sigemptyset(&s.sa_mask);
    s.sa_sigaction = (void (*)(int, siginfo_t *, void *))signal_handler;
    sigaction(SIGSEGV, &s, &u);
}