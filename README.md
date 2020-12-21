# CMPSC473-OS-Design #


## Description
CMPSC 473 Operating system design project from Penn State University

## Project 1: Thread Schedulers
Project 1 implements four thread schedulers for a uni-processor-environment with support for semaphore synchronization:
1. First Come First Served (FCFS) - no preemption
2. Shortest Remaining Time First (SRTF) - with preemption
3. Priority Based Scheduling (PBS) - with preemption
4. Multi Level Feedback Queue (MLFQ) - with preemption and aging.

- P: to wait on a counting semaphore, blocking till the corresponding semaphore is signalled or has been signalled
- V: to signal a semaphore (signal), and in the process waking up any waiting thread.

## Project 2: Memory Allocation
Project 2 implements two memory allocation/de-allocation schemes: Buddy System and Slab Allocation
### Buddy Allocator
The minimum chunk of memory that the Buddy System can allocate is 1KB.
Maintain the free (holes) list sorted by increasing addresses individually for different sizes (of powers of 2)
Whenever there is a choice of multiple holes that a scheme allows, always pick the hole with the lowest starting address.
The 4 byte header is used to maintain the size of the allocated chunk within the chunk itself..
### Slab Allocator
The Slab size is fixed at N_OBJS_PER_SLAB which is always 64 and is defined in my_memory.c. Note that each slab will only contain the objects of the appropriate type/size. However the size of the allocated object itself should be accounted to include its 4 byte header. Hence, when using this scheme for allocating an object, there will be a 4 byte header for the object, and additionally a 4 byte header in the slab itself (where this object resides) which is allocated using Buddy.
The Slab Descriptor Table itself is maintained as a separate data structure.


## Project 3: Virtual Memory - Paging
Project 3 implements software algorithms for managing the virtual pages within a limited physical memory and following mechanisms:
- (i) an access control mechanism for the former where the hardware will directly do the translations/accesses for the cases where pages are already in memory, and fault over to the software when the page is not resident
- (ii) First-In-First-Out Replacement and Third chance Replacement (a variant of the 2nd chance replacement algorithm) to make room for newly incoming pages into physical memory.
