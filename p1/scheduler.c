/**
 * 
 * File             : scheduler.c
 * Description      : This is a stub to implement all your scheduling schemes
 *
 * Author(s)        : Sung Woo Oh, Jihoon (Jeremiah) Park
*/

// clang-format off
// Include Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include <math.h>
#include <pthread.h>

void init_scheduler( int sched_type );
int schedule_me( float currentTime, int tid, int remainingTime, int tprio );
int P( float currentTime, int tid, int sem_id);
int V( float currentTime, int tid, int sem_id);

#define FCFS    0
#define SRTF    1
#define PBS     2
#define MLFQ    3
#define SEM_ID_MAX 50

#define TNUM 10

//////////////////////////
/////// QUEUE Info ///////
//////////////////////////

// Thread Queue
typedef struct queue{
    int currentid;
    float currentTime;
    int remainingTime;
    int prio;
    float arrivalTime;
    int semid;
	int isBlocked; // 0: false 1: true

    struct queue *next;
}queue;

// Semaphore Queue / Blocked Queue
typedef struct blockedQ{
	int currentid;
    float currentTime;
    int remainingTime;
    int prio;
    float arrivalTime;
    int semid;
 	int semCount; // 0: wait 1: signal
	
	int level;
	int age;

	struct blockedQ *next;
}blockedQ;

// Multi Level Feed Back Queue
typedef struct multiQ{
	int currentid;
    float currentTime;
    int remainingTime;
    int prio;
    float arrivalTime;
    int semid;
 	int semCount; // 0: wait 1: signal
	//  MLFQ special vars
	int level;
	int age;

	struct multiQ *next;
}multiQ;

//////////////////////////
//// Global variables ////
//////////////////////////

queue *qinfoRunning;
queue *qinfoReady;
blockedQ *blockedInfo[50];
multiQ *multiInfo[5];
multiQ *qinfoRunningMLFQ;

pthread_mutex_t mutexCond;
pthread_mutex_t mutex_update_tid; //inner (updateQ 할때)
pthread_mutex_t mutex_adddel_tid; //outer (RunningQ랑 ReadyQ 동시에 못들어가게)
pthread_mutex_t mutex_rem0_tid; //outer (RunningQ랑 ReadyQ 동시에 못들어가게)
pthread_cond_t condFCFS;
pthread_cond_t cond[TNUM];


int option;
double globalCurrentTime;


////////////////////////////////////////////////////////////////////////////////
//
// Function     : Helper functions
//
// Description  : updateQ, checkStatus
//

void sorting(queue **ptr){
	queue *prev = (*ptr);
	queue *curr = (*ptr)->next;

	while(curr != NULL){
		// Compare Remaining Time 비교
		if(curr->remainingTime < prev->remainingTime){
			// Remaining Time 비교해서 원래 curr이 맨앞으로 curr있던자리는 prev가 prev다음을 curr로
			if(curr->currentTime != prev->currentTime){
				prev->next = curr->next;
				curr->next = (*ptr);
				(*ptr) = curr;
				curr = prev;
			}
			// 만약 Remainint Time 같을때면 Current Time 더 낮은게 먼저
			else{
				if (curr->currentTime < prev->currentTime){
					prev->next = curr->next;
					curr->next = (*ptr);
					(*ptr) = curr;
					curr = prev;
				}
			}
			
		}
		else{
			prev = curr;
		}

		curr = curr->next;
	}
}

void sortingPBS(queue **ptr){
	queue *prev = (*ptr);
	queue *curr = (*ptr)->next;

	while(curr != NULL){
		// Compare prio 비교
		if(curr->prio < prev->prio){
			// prio 비교해서 원래 curr이 맨앞으로 curr있던자리는 prev가 prev다음을 curr로
			if(curr->currentTime != prev->currentTime){
				prev->next = curr->next;
				curr->next = (*ptr);
				(*ptr) = curr;
				curr = prev;
			}
			// 만약 prio 같을때면 Current Time 더 낮은게 먼저
			else{
				if (curr->currentTime < prev->currentTime){
					prev->next = curr->next;
					curr->next = (*ptr);
					(*ptr) = curr;
					curr = prev;
				}
			}
			
		}
		else{
			prev = curr;
		}

		curr = curr->next;
	}
}

void updateQ(queue *ptr, int tid, float currentTime, int tprio, int remainingTime, float arrivalTime){
	ptr->currentid = tid;
	ptr->currentTime = currentTime;
	ptr->prio = tprio;
	ptr->remainingTime = remainingTime;

	if(arrivalTime == -1){
		ptr->arrivalTime = currentTime;
	}
	else{
		ptr->arrivalTime = arrivalTime;
	}
}

void updateBQ(blockedQ *ptr, int tid, float currentTime, int tprio, int remainingTime, float arrivalTime){
	ptr->currentid = tid;
	ptr->currentTime = currentTime;
	ptr->prio = tprio;
	ptr->remainingTime = remainingTime;

	if(arrivalTime == -1){
		ptr->arrivalTime = currentTime;
	}
	else{
		ptr->arrivalTime = arrivalTime;
	}
}

void updateBlockedMLFQ(blockedQ *ptr, int tid, float currentTime, int tprio, int remainingTime, float arrivalTime, int level, int age){
	ptr->currentid = tid;
	ptr->currentTime = currentTime;
	ptr->prio = tprio;
	ptr->remainingTime = remainingTime;
	ptr->level = level;
	ptr->age = age;

	if(arrivalTime == -1){
		ptr->arrivalTime = currentTime;
	}
	else{
		ptr->arrivalTime = arrivalTime;
	}
}

void updateMLFQ(multiQ *ptr, int tid, float currentTime, int tprio, int remainingTime, float arrivalTime, int level, int age){
	ptr->currentid = tid;
	ptr->currentTime = currentTime;
	ptr->prio = tprio;
	ptr->remainingTime = remainingTime;
	ptr->level = level;
	ptr->age = age;

	if(arrivalTime == -1){
		ptr->arrivalTime = currentTime;
	}
	else{
		ptr->arrivalTime = arrivalTime;
	}
}

void checkStatus(queue *ptr, int queueNum){
	// Running
	if (queueNum == 0){
		while(ptr != NULL){
			if(ptr->currentid == 0){
				printf("-------- Empty Running Queue --------\n");
				ptr = ptr->next;
			}
			else{
				printf("-------- In RunningQ)   T%d  CurrentTime: %f  RemainingTime: %d --------\n", ptr->currentid, ptr->currentTime, ptr->remainingTime);
				ptr = ptr->next;
			}
		}
	}
   	else{ //ready
		int num = 1;
		while(ptr != NULL){
			if(ptr->currentid == 0){
				printf("-------- Empty Ready Queue --------\n");
				ptr = ptr->next;
			}
			else{
				printf("-------- In ReadyQ%d)   T%d  CurrentTime: %f  RemainingTime: %d --------\n", num, ptr->currentid, ptr->currentTime, ptr->remainingTime);
				ptr = ptr->next;
				num += 1;
			}
		}
   }
}



////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_scheduler
//
// Input        : sched_type
//
// Description  : 0-FCFS, 1-SRTF, 2-PBS, 3-MLFQ
//

void init_scheduler(int sched_type){

	
	pthread_mutex_init(&mutex_update_tid, NULL);
	pthread_mutex_init(&mutex_adddel_tid, NULL);
	pthread_mutex_init(&mutex_rem0_tid, NULL);
	pthread_mutex_init(&mutexCond, NULL);

	// Cond var per each Thread
	for(int i=0; i<TNUM; i++){
		pthread_cond_init(&cond[i], NULL);
	}

	// set initial current time to 0
	globalCurrentTime = 0;

	// allocate space for thread and queue
	qinfoRunning = malloc(sizeof(queue));
	qinfoReady = malloc(sizeof(queue));
	qinfoRunningMLFQ = malloc(sizeof(multiQ));

	// initialize running queue
	qinfoRunning->currentid = 0;
	qinfoRunning->currentTime = 0;
	qinfoRunning->prio = 0;
	qinfoRunning->next = NULL;

	// initialize ready queue
	qinfoReady->currentid = 0;
	qinfoReady->currentTime = 0;
	qinfoReady->prio = 0;
	qinfoReady->next = NULL;

	// initialize arrary of semaphore queues
	for(int i=0; i<SEM_ID_MAX; i++){
		blockedInfo[i] = malloc(sizeof(blockedQ));
		blockedInfo[i]->currentid = 0;
		blockedInfo[i]->currentTime = 0;
		blockedInfo[i]->remainingTime = 0;
		blockedInfo[i]->prio = 0;
		blockedInfo[i]->arrivalTime = 0;
		blockedInfo[i]->semid = 0;
		blockedInfo[i]->semCount = 0;
		blockedInfo[i]->level = 0;
		blockedInfo[i]->age = 0;

	}
	// initialize arrary of multilevel queues for MLFQ
	for(int i=0; i<5; i++){
		multiInfo[i] = malloc(sizeof(multiQ));
		multiInfo[i]->currentid = 0;
		multiInfo[i]->currentTime = 0;
		multiInfo[i]->remainingTime = 0;
		multiInfo[i]->prio = 0;
		multiInfo[i]->arrivalTime = 0;
		multiInfo[i]->semid = 0;
		multiInfo[i]->semCount = 0;
		multiInfo[i]->level = 0;
		multiInfo[i]->age = 0;
	}

	// initialize running queue in MLFQ
	qinfoRunningMLFQ->currentid = 0;
	qinfoRunningMLFQ->currentTime = 0;
	qinfoRunningMLFQ->remainingTime = 0;
	qinfoRunningMLFQ->prio = 0;
	qinfoRunningMLFQ->arrivalTime = 0;
	qinfoRunningMLFQ->semid = 0;
	qinfoRunningMLFQ->semCount = 0;
	qinfoRunningMLFQ->level = 0;
	qinfoRunningMLFQ->age = 0;
	
	// Pick the schedule option
	switch(sched_type){
		case 0: option = 0; break;
		case 1: option = 1; break;
		case 2: option = 2; break;
		case 3: option = 3; break;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : schedule_me
//
// Input        : currentTime, tid, remainingTime, tprio
//
// Description  : A thread will call this function for every timer tick, 
//		          with the current time and the remaining time in its CPU burst. 
//				  You should return from this function only when this thread has 
//				  got the CPU until the next timer tick. 
//				  Else it should block until it does. As it keeps getting the CPU, 
//				  the remaining time in its burst will accordingly reduce. 
//				  This function should implement the actual scheduler functionality 
//				  based on the initialized scheduler type in the init_scheduler() function.
//


int schedule_me(float currentTime, int tid, int remainingTime, int tprio)
{

	globalCurrentTime = currentTime;
	
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////
	///////////////////////// FCFS /////////////////////////
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////

	if (option == FCFS){
		
        pthread_mutex_lock(&mutex_adddel_tid); //들어가는거 

		// When nothing's in running & ready Queue
		if (qinfoRunning->currentid == 0 && qinfoReady->currentid == 0){
            // insert running for the first tid if running queue and ready queue is NULL
            pthread_mutex_lock(&mutex_update_tid);
			updateQ(qinfoRunning, tid, currentTime, tprio, remainingTime, -1);
			pthread_mutex_unlock(&mutex_update_tid);
        }
		// 똑같은애 돌리는거
		else if(qinfoRunning->currentid == tid){
			//update currentime and remaining time if tid is still running
            qinfoRunning->currentTime = currentTime;
            qinfoRunning->remainingTime = remainingTime;
		}
		// 다른애가 찾아올때
		else if(qinfoRunning->currentid != tid){

			// 다른애가 찾아왔는데 ready queue에 아무거도 없을때
			if (qinfoReady->currentid == 0){
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoReady, tid, currentTime, tprio, remainingTime, -1);
				pthread_mutex_unlock(&mutex_update_tid);
			}
			// 다른애가 찾아왔는데 ready queue에 누가 있을때
			else{
				queue *readyTail = qinfoReady; //check if tid is in ready queue

				// tail 로 point 하게끔 룹
				while(readyTail->next != NULL){
					// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
					if (readyTail->currentid == tid){
						break;
					}
					// 새로운 ready queue
					else{
						readyTail = readyTail->next;
					}
				}
				// INSERT: tail 에서 새로운 애가 들어왔을때 새로운 node 생성 (temp)
				if(readyTail->next == NULL && readyTail->currentid != tid){
					pthread_mutex_lock(&mutex_update_tid);
					queue *temp = malloc(sizeof(queue));
					updateQ(temp, tid, currentTime, tprio, remainingTime, -1);
					temp->next = NULL;
					readyTail->next = temp;
					pthread_mutex_unlock(&mutex_update_tid);
				}
			}
		}
		pthread_mutex_unlock(&mutex_adddel_tid);

		//////////////////////////////////////////////////// MOVE TO ANOTHER QUEUE /////////////////////////////////////////////////////////////////
		
		// tid 다른애 일단 block. 
		if (tid != qinfoRunning->currentid){
			pthread_mutex_lock(&mutexCond);
			pthread_cond_wait(&condFCFS, &mutexCond);
			pthread_mutex_unlock(&mutexCond);
		}

		// running 하고 있던애가 끝나면
		if(qinfoRunning->remainingTime == 0){
			// this is when we choose next running tid from queue
			// and move that tid to running tid
			pthread_mutex_lock(&mutex_adddel_tid);
			// running 도 끝나고 ready queue에도 아무도 없으면 0으로 초기화
			if(qinfoReady->currentid == 0){
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoRunning, 0, 0, 0, 0, -1);
				pthread_mutex_unlock(&mutex_update_tid);
			}
			else{
				// MOVE TO ANOTHER QUEUE: tid ready에서 running 으로 (기존에있던 자리는 0으로 초기화) 
				// ready queue에 tid가 하나밖에 없을때
				if((qinfoReady->next == NULL)){
					pthread_mutex_lock(&mutex_update_tid);
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
					updateQ(qinfoReady, 0, 0, 0, 0, -1);
					pthread_mutex_unlock(&mutex_update_tid);
				}
				// ready queue에 tid가 여러개 있을때
				else{
					pthread_mutex_lock(&mutex_update_tid);
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);

					// ready queue안에 여러게 있을때 head를 free 지우고 그 다음 노드로 업데이트
					if (qinfoReady->next != NULL){
						queue *theTemp;
						theTemp = qinfoReady;
						qinfoReady = qinfoReady->next;
						theTemp->next = NULL;
						free(theTemp);
					}
					pthread_mutex_unlock(&mutex_update_tid);
				}
			}

			// block 한애 signal 주기
			pthread_mutex_lock(&mutexCond);
			pthread_cond_signal(&condFCFS);
			pthread_mutex_unlock(&mutexCond);



			pthread_mutex_unlock(&mutex_update_tid);
			return globalCurrentTime;
		}
		pthread_mutex_unlock(&mutex_adddel_tid);
		
        // checkStatus(qinfoRunning, 0);
		// checkStatus(qinfoReady, 1);
		
		return globalCurrentTime;
	}

	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////
	///////////////////////// SRTF /////////////////////////
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////

	else if(option == SRTF){

		pthread_mutex_lock(&mutex_adddel_tid); //들어가는거 

		// When nothing's in running & ready Queue
		if (qinfoRunning->currentid == 0 && qinfoReady->currentid == 0){
            // insert running for the first tid if running queue and ready queue is NULL
            pthread_mutex_lock(&mutex_update_tid);
			updateQ(qinfoRunning, tid, currentTime, tprio, remainingTime, -1);
			pthread_mutex_unlock(&mutex_update_tid);
        }
		// 똑같은애 돌리는거
		else if(qinfoRunning->currentid == tid){
			//update currentime and remaining time if tid is still running
            qinfoRunning->currentTime = currentTime;
            qinfoRunning->remainingTime = remainingTime;
		}
		// 다른애가 찾아올때
		else if(qinfoRunning->currentid != tid){

			// 다른애가 찾아왔는데 ready queue에 아무거도 없을때
			if (qinfoReady->currentid == 0){
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoReady, tid, currentTime, tprio, remainingTime, -1);
				if(qinfoRunning->remainingTime > qinfoReady->remainingTime){
					queue *temp = malloc(sizeof(queue));
					updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
					updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);
					free(temp);
					sorting(&qinfoReady);
				}
				else if(qinfoRunning->remainingTime == qinfoReady->remainingTime){
					if(qinfoRunning->currentTime > qinfoReady->currentTime){
						queue *temp = malloc(sizeof(queue));
						updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
						updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
						updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);
						free(temp);
						sorting(&qinfoReady);
					}
				}
				pthread_mutex_unlock(&mutex_update_tid);
			}
			// 다른애가 찾아왔는데 ready queue에 누가 있을때
			else{
				queue *readyTail = qinfoReady; //check if tid is in ready queue

				// tail 로 point 하게끔 룹
				while(readyTail->next != NULL){
					// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
					if (readyTail->currentid == tid){
						break;
					}
					// 새로운 ready queue
					else{
						readyTail = readyTail->next;
					}
				}
				// INSERT: tail 에서 새로운 애가 들어왔을때 새로운 node 생성 (temp)
				if(readyTail->next == NULL && readyTail->currentid != tid){
					pthread_mutex_lock(&mutex_update_tid);
					queue *temp = malloc(sizeof(queue));
					updateQ(temp, tid, currentTime, tprio, remainingTime, -1);
					temp->next = NULL;
					readyTail->next = temp;
					sorting(&qinfoReady);
					if (qinfoReady->currentid != 0){
						
						updateQ(qinfoReady, tid, currentTime, tprio, remainingTime, -1);
						if(qinfoRunning->remainingTime > qinfoReady->remainingTime){
							queue *temp = malloc(sizeof(queue));
							updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
							updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
							updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);
							free(temp);
							sorting(&qinfoReady);
						}
						else if(qinfoRunning->remainingTime == qinfoReady->remainingTime){
							if(qinfoRunning->currentTime > qinfoReady->currentTime){
								queue *temp = malloc(sizeof(queue));
								updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
								updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
								updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);	
								free(temp);
								sorting(&qinfoReady);
							}
						}
					}
					pthread_mutex_unlock(&mutex_update_tid);
				}
			}
		}
		pthread_mutex_unlock(&mutex_adddel_tid);

		//////////////////////////////////////////////////// MOVE TO ANOTHER QUEUE /////////////////////////////////////////////////////////////////
		
		// tid 다른애 일단 block. 
		if (tid != qinfoRunning->currentid){
			pthread_mutex_lock(&mutexCond);
			pthread_cond_wait(&cond[tid], &mutexCond);
			pthread_mutex_unlock(&mutexCond);
		}
		// else{
		// 	pthread_cond_signal(&cond[qinfoRunning->currentid]);
		// }
		
		
		// running 하고 있던애가 끝나면
		if(qinfoRunning->remainingTime == 0){
			// this is when we choose next running tid from queue
			// and move that tid to running tid
			pthread_mutex_lock(&mutex_adddel_tid);
			// running 도 끝나고 ready queue에도 아무도 없으면 0으로 초기화
			if(qinfoReady->currentid == 0){
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoRunning, 0, 0, 0, 0, -1);
				pthread_mutex_unlock(&mutex_update_tid);
			}
			else{
				// MOVE TO ANOTHER QUEUE: tid ready에서 running 으로 (기존에있던 자리는 0으로 초기화) 
				// ready queue에 tid가 하나밖에 없을때
				if((qinfoReady->next == NULL)){
					pthread_mutex_lock(&mutex_update_tid);
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
					updateQ(qinfoReady, 0, 0, 0, 0, -1);
					pthread_mutex_unlock(&mutex_update_tid);
				}
				// ready queue에 tid가 여러개 있을때
				else{
					pthread_mutex_lock(&mutex_update_tid);
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);

					// ready queue안에 여러게 있을때 head를 free 지우고 그 다음 노드로 업데이트
					if (qinfoReady->next != NULL){
						queue *theTemp;
						theTemp = qinfoReady;
						qinfoReady = qinfoReady->next;
						theTemp->next = NULL;
						free(theTemp);
					}
					pthread_mutex_unlock(&mutex_update_tid);
				}
				sorting(&qinfoReady);
			}

			// block 한애 signal 주기
			pthread_mutex_lock(&mutexCond);
			pthread_cond_signal(&cond[qinfoRunning->currentid]);
			pthread_mutex_unlock(&mutexCond);
			
			

			pthread_mutex_unlock(&mutex_update_tid);
			pthread_mutex_unlock(&mutex_adddel_tid);
			// return globalCurrentTime;
		}
		
		
        // checkStatus(qinfoRunning, 0);
		// checkStatus(qinfoReady, 1);
		
		return globalCurrentTime;
	}
	
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////
	///////////////////////// PBS //////////////////////////
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////

	else if(option == PBS){
		
		pthread_mutex_lock(&mutex_update_tid);
		////************************************************** 똑같은애 들어올때 업데이트 ***********************************************************************////
		if(qinfoRunning->currentid == tid){
			//update currentime and remaining time if tid is still running
            qinfoRunning->currentTime = currentTime;
            qinfoRunning->remainingTime = remainingTime;
		}
		///////////////////////////////////////////////////////// 똑같은애 들어올때 업데이트 ////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&mutex_update_tid);


		pthread_mutex_lock(&mutex_adddel_tid);
		////************************************************** 처음에 들어올때 ***********************************************************************////

		// Running & Ready에 아무도 없을때
		if (qinfoRunning->currentid == 0 && qinfoReady->currentid == 0){
            // insert running for the first tid if running queue and ready queue is NULL  
			updateQ(qinfoRunning, tid, currentTime, tprio, remainingTime, -1);
        }
		///////////////////////////////////////////////////////// 처음에 들어올때  ////////////////////////////////////////////////////////////////////




		////************************************************** 다른애가 찾아왔는데 ready queue에 아무거도 없을때 ***********************************************************************////
		if(qinfoRunning->currentid != tid && qinfoReady->currentid == 0){

			//@@@@@@@@@@@@@@@@@ Running Q priority 랑 비교해서 바꿈 @@@@@@@@@@@@@@@@@ //

			updateQ(qinfoReady, tid, currentTime, tprio, remainingTime, -1);
			if(qinfoRunning->prio > qinfoReady->prio){
				queue *temp = malloc(sizeof(queue));
				updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
				updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
				updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);
				free(temp);	
				sortingPBS(&qinfoReady);
			}
			else if(qinfoRunning->prio == qinfoReady->prio){
				if(qinfoRunning->currentTime > qinfoReady->currentTime){
					queue *temp = malloc(sizeof(queue));
					updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
					updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);
					free(temp);
					sortingPBS(&qinfoReady);
				}
			}
		}
		///////////////////////////////////////////////////////// 다른애가 찾아왔는데 ready queue에 아무거도 없을때  ////////////////////////////////////////////////////////////////////

		


		// //************************************************** 다른애가 찾아왔는데 ready queue에 누가 있을때 ***********************************************************************////
		if(qinfoRunning->currentid != tid && qinfoReady->currentid != 0){
			queue *readyTail = qinfoReady; //check if tid is in ready queue

			// tail 로 point 하게끔 룹
			while(readyTail->next != NULL){
				// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
				if (readyTail->currentid == tid){
					break;
				}
				// 새로운 ready queue
				else{
					readyTail = readyTail->next;
				}
			}
			// INSERT: tail 에서 새로운 애가 들어왔을때 새로운 node 생성 (temp)
			if(readyTail->next == NULL && readyTail->currentid != tid){
				
				queue *temp = malloc(sizeof(queue));
				updateQ(temp, tid, currentTime, tprio, remainingTime, -1);
				temp->next = NULL;
				readyTail->next = temp;
				sortingPBS(&qinfoReady);
				if (qinfoReady->currentid != 0){
					
					updateQ(qinfoReady, tid, currentTime, tprio, remainingTime, -1);

					//@@@@@@@@@@@@@@@@@ Running Q priority 랑 비교해서 바꿈 @@@@@@@@@@@@@@@@@ //
					if(qinfoRunning->prio > qinfoReady->prio){
						queue *temp = malloc(sizeof(queue));
						updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
						updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
						updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);
						free(temp);
						sortingPBS(&qinfoReady);
					}
					else if(qinfoRunning->prio == qinfoReady->prio){
						if(qinfoRunning->currentTime > qinfoReady->currentTime){
							queue *temp = malloc(sizeof(queue));
							updateQ(temp, qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
							updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
							updateQ(qinfoReady, temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime);	
							free(temp);
							sortingPBS(&qinfoReady);
						}
					}
				}
			}
		}
		///////////////////////////////////////////////////////// 다른애가 찾아왔는데 ready queue에 누가 있을때  ////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&mutex_adddel_tid);
	


		// tid 다른애 일단 block. 
		if (tid != qinfoRunning->currentid){
			pthread_mutex_lock(&mutexCond);
			pthread_cond_wait(&cond[tid], &mutexCond);
			pthread_mutex_unlock(&mutexCond);
		}

		pthread_mutex_lock(&mutex_rem0_tid);
		//************************************************** running 하고 있던애가 끝나면 ***********************************************************************////
		if(qinfoRunning->remainingTime == 0){
			// this is when we choose next running tid from queue
			// and move that tid to running tid
			// pthread_mutex_lock(&mutex_adddel_tid);
			// running 도 끝나고 ready queue에도 아무도 없으면 0으로 초기화
			if(qinfoReady->currentid == 0){
				updateQ(qinfoRunning, 0, 0, 0, 0, -1);
			}
			else{
				// MOVE TO ANOTHER QUEUE: tid ready에서 running 으로 (기존에있던 자리는 0으로 초기화) 
				// ready queue에 tid가 하나밖에 없을때
				if((qinfoReady->next == NULL)){
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
					updateQ(qinfoReady, 0, 0, 0, 0, -1);
				}
				// ready queue에 tid가 여러개 있을때
				else{
					updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
					// ready queue안에 여러게 있을때 head를 free 지우고 그 다음 노드로 업데이트
					if (qinfoReady->next != NULL){
						queue *theTemp;
						theTemp = qinfoReady;
						qinfoReady = qinfoReady->next;
						theTemp->next = NULL;
						free(theTemp);
					}
				}
				sortingPBS(&qinfoReady);
			}
			
		}
		///////////////////////////////////////////////////////// running 하고 있던애가 끝나면  ////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&mutex_rem0_tid);
		
		// block 한애 signal 주기
			pthread_mutex_lock(&mutexCond);
			pthread_cond_signal(&cond[qinfoRunning->currentid]);
			pthread_mutex_unlock(&mutexCond);

		return globalCurrentTime;
	}
	
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////
	///////////////////////// MLFQ /////////////////////////
	////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////

	else if(option == MLFQ){
		
		pthread_mutex_lock(&mutex_update_tid);
		////************************************************** 똑같은애 들어올때 업데이트 ***********************************************************************////

		if(qinfoRunningMLFQ->currentid == tid){
			//update currentime and remaining time if tid is still running
            qinfoRunningMLFQ->currentTime = currentTime;
            qinfoRunningMLFQ->remainingTime = remainingTime;
			qinfoRunningMLFQ->age = qinfoRunningMLFQ->age - 1;

			
			// Age 유통기한 만료되면 밑에 레벨로 이동
			if (qinfoRunningMLFQ->age == 0){
				qinfoRunningMLFQ->level += 1;
				qinfoRunningMLFQ->age = qinfoRunningMLFQ->level*5 + 5;
				// 자기 레벨 Ready Q 에 아무도 없을때
				if(multiInfo[qinfoRunningMLFQ->level]->currentid == 0){
					updateMLFQ(multiInfo[qinfoRunningMLFQ->level], qinfoRunningMLFQ->currentid, qinfoRunningMLFQ->currentTime, qinfoRunningMLFQ->prio, qinfoRunningMLFQ->remainingTime, qinfoRunningMLFQ->arrivalTime, qinfoRunningMLFQ->level, qinfoRunningMLFQ->age);
				}
			
				// 자기 레벨 Ready Q 에 누가 있을때
				else if(multiInfo[qinfoRunningMLFQ->level]->currentid != 0){
					
					//*************** 자기레벨 Ready Q 맨뒤에 붙이는거 ****************//
					// tail 로 point 하게끔 룹

					multiQ *readyTail; 
					readyTail = multiInfo[qinfoRunningMLFQ->level];
					while(readyTail->next != NULL){
						// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
						if (readyTail->currentid == qinfoRunningMLFQ->currentid){
							break;
						}
						// 새로운 ready queue
						else{
							readyTail = readyTail->next;
						}
					}

					if(readyTail->next == NULL && readyTail->currentid != qinfoRunningMLFQ->currentid){
						multiQ *temp = malloc(sizeof(multiQ));
						updateMLFQ(temp, qinfoRunningMLFQ->currentid, qinfoRunningMLFQ->currentTime, qinfoRunningMLFQ->prio, qinfoRunningMLFQ->remainingTime, qinfoRunningMLFQ->arrivalTime, qinfoRunningMLFQ->level, qinfoRunningMLFQ->age);
						temp->next = NULL;
						readyTail->next = temp;
					}
					/////////////// 자기레벨 Ready Q 맨뒤에 붙이는거 ///////////////
				}

				//***************** Running Q 들어갈애 뽑기 *****************//
				for(int i = 0; i < 5; i++){
					// Ready Q 에 누가 있을때
					if(multiInfo[i]->currentid != 0){
						// Ready Q 에 한명만 있을때
						if(multiInfo[i]->next == NULL){
							updateMLFQ(qinfoRunningMLFQ, multiInfo[i]->currentid, multiInfo[i]->currentTime, multiInfo[i]->prio, multiInfo[i]->remainingTime, multiInfo[i]->arrivalTime, multiInfo[i]->level, multiInfo[i]->age);
							updateMLFQ(multiInfo[i], 0, 0, 0, 0, 0, 0, 0);
						}
						// Ready Q 에 여러명 있을때 맨앞애를 Running 으로 옮겨주고 전체 한칸씩 앞으로 땡기기
						else{
							multiQ *temp = multiInfo[i]->next;
							updateMLFQ(qinfoRunningMLFQ, multiInfo[i]->currentid, multiInfo[i]->currentTime, multiInfo[i]->prio, multiInfo[i]->remainingTime, multiInfo[i]->arrivalTime, multiInfo[i]->level, multiInfo[i]->age);
							updateMLFQ(multiInfo[i], temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime, temp->level, temp->age);
							multiInfo[i]->next = temp->next;
							temp->next = NULL;
							free(temp);
						}
						break;
					}
				}
				pthread_cond_signal(&cond[qinfoRunningMLFQ->currentid]);
				////////////////// Running Q 들어갈애 뽑기 /////////////////
			}
		}
		///////////////////////////////////////////////////////// 똑같은애 들어올때 업데이트 ////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&mutex_update_tid);



		pthread_mutex_lock(&mutex_adddel_tid);
		////************************************************** 처음에 들어올때, Running & Ready 첫칸에 아무도 없을때 ***********************************************************************////
		//Running & Ready 첫칸에 아무도 없을때
		if(qinfoRunningMLFQ->currentid == 0 && multiInfo[0]->currentid == 0){
			updateMLFQ(qinfoRunningMLFQ, tid, currentTime, tprio, remainingTime, -1, 0, 5);
		}
		///////////////////////////////////////////////////////// 처음에 들어올때  ////////////////////////////////////////////////////////////////////
		

		////************************************************** 다른애가 찾아왔는데 ready queue에 아무거도 없을때 ***********************************************************************////
		else if(qinfoRunningMLFQ->currentid != tid && multiInfo[0]->currentid == 0){
			updateMLFQ(multiInfo[0], tid, currentTime, tprio, remainingTime, -1, 0, 5);
			
		}
		///////////////////////////////////////////////////////// 다른애가 찾아왔는데 ready queue에 아무거도 없을때  ////////////////////////////////////////////////////////////////////

		// //************************************************** 다른애가 찾아왔는데 ready queue에 누가 있을때 ***********************************************************************////
		else if(qinfoRunningMLFQ->currentid != tid && multiInfo[0]->currentid != 0){
		
			multiQ *readyTail0 = multiInfo[0];
			multiQ *readyTail1 = multiInfo[1];
			multiQ *readyTail2 = multiInfo[2];
			multiQ *readyTail3 = multiInfo[3];
			multiQ *readyTail4 = multiInfo[4];
				//check if tid is in ready queue
			// tail 로 point 하게끔 룹
			int exist = 0;
			if (tid == readyTail0->currentid){
				exist = 1;
			}
			if (tid == readyTail1->currentid){
				exist = 1;
			}
			if (tid == readyTail2->currentid){
				exist = 1;
			}
			if (tid == readyTail3->currentid){
				exist = 1;
			}
			if (tid == readyTail4->currentid){
				exist = 1;
			}
			while(readyTail0->next != NULL){
				// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
				if (readyTail0->currentid == tid){
					exist = 1;
					break;
				}
				// 새로운 ready queue
				else{
					readyTail0 = readyTail0->next;
				}
			}
			while(readyTail1->next != NULL){
				// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
				if (readyTail1->currentid == tid){
					exist = 1;
					break;
				}
				// 새로운 ready queue
				else{
					readyTail1 = readyTail1->next;
				}
			}
			while(readyTail2->next != NULL){
				// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
				if (readyTail2->currentid == tid){
					exist = 1;
					break;
				}
				// 새로운 ready queue
				else{
					readyTail2 = readyTail2->next;
				}
			}
			while(readyTail3->next != NULL){
				// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
				if (readyTail3->currentid == tid){
					exist = 1;
					break;
				}
				// 새로운 ready queue
				else{
					readyTail3 = readyTail3->next;
				}
			}
			while(readyTail4->next != NULL){
				// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
				if (readyTail4->currentid == tid){
					exist = 1;
					break;
				}
				// 새로운 ready queue
				else{
					readyTail4 = readyTail4->next;
				}
			}
			// INSERT: tail 에서 새로운 애가 들어왔을때 새로운 node 생성 (temp)
			if(exist == 0){
				if(readyTail0->next == NULL && readyTail0->currentid != tid && readyTail0->remainingTime != 0){
					multiQ *temp = malloc(sizeof(multiQ));
					updateMLFQ(temp, tid, currentTime, tprio, remainingTime, -1, 0 ,5);
					temp->next = NULL;
					readyTail0->next = temp;
				}
			}
		}
		///////////////////////////////////////////////////////// 다른애가 찾아왔는데 ready queue에 누가 있을때  ////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&mutex_adddel_tid);

		// tid 다른애 일단 block. 
		if (tid != qinfoRunningMLFQ->currentid){
			pthread_mutex_lock(&mutexCond);
			pthread_cond_wait(&cond[tid], &mutexCond);
			pthread_mutex_unlock(&mutexCond);
			// pthread_cond_signal(&cond[qinfoRunningMLFQ->currentid]);
		}

		pthread_mutex_lock(&mutex_rem0_tid);
		//remaining Time 이 끝날 때
		if(qinfoRunningMLFQ->remainingTime == 0){
			
			//***************** Running Q 들어갈애 뽑기 *****************//
			for(int i = 0; i < 5; i++){
				// Ready Q 에 누가 있을때
				if(multiInfo[i]->currentid != 0){
					// Ready Q 에 한명만 있을때
					if(multiInfo[i]->next == NULL){
						updateMLFQ(qinfoRunningMLFQ, multiInfo[i]->currentid, multiInfo[i]->currentTime, multiInfo[i]->prio, multiInfo[i]->remainingTime, multiInfo[i]->arrivalTime, multiInfo[i]->level, multiInfo[i]->age);
						updateMLFQ(multiInfo[i], 0, 0, 0, 0, 0, 0, 0);
					}
					// Ready Q 에 여러명 있을때 맨앞애를 Running 으로 옮겨주고 전체 한칸씩 앞으로 땡기기
					else{
						multiQ *temp = multiInfo[i]->next;
						updateMLFQ(qinfoRunningMLFQ, multiInfo[i]->currentid, multiInfo[i]->currentTime, multiInfo[i]->prio, multiInfo[i]->remainingTime, multiInfo[i]->arrivalTime, multiInfo[i]->level, multiInfo[i]->age);
						updateMLFQ(multiInfo[i], temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime, temp->level, temp->age);
						multiInfo[i]->next = temp->next;
						temp->next = NULL;
						free(temp);
					}
					break;
				}
			}
			////////////////// Running Q 들어갈애 뽑기 /////////////////
				
		
		}
		// block 한애 signal 주기
		pthread_mutex_lock(&mutexCond);
		pthread_cond_signal(&cond[qinfoRunningMLFQ->currentid]);
		pthread_mutex_unlock(&mutexCond);
		pthread_mutex_unlock(&mutex_rem0_tid);
		
		return globalCurrentTime;
	}
}

int P( float currentTime, int tid, int sem_id) { // returns current global time

	globalCurrentTime = currentTime;
	
	if(blockedInfo[sem_id]->semCount > 0){
		blockedInfo[sem_id]->semCount--;
	}
	else{
		//Currently running Thread -> Block Queue 이동
		if(option == 0 || option == 1 || option == 2){
			updateBQ(blockedInfo[sem_id], qinfoRunning->currentid, qinfoRunning->currentTime, qinfoRunning->prio, qinfoRunning->remainingTime, qinfoRunning->arrivalTime);
			// Ready Queue head -> Running Queue 이동
			// MOVE TO ANOTHER QUEUE: tid ready에서 running 으로 (기존에있던 자리는 0으로 초기화) 
			// ready queue에 tid가 하나밖에 없을때
			if((qinfoReady->next == NULL)){
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);
				updateQ(qinfoReady, 0, 0, 0, 0, -1);
				pthread_mutex_unlock(&mutex_update_tid);
			}
			// ready queue에 tid가 여러개 있을때
			else{
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoRunning, qinfoReady->currentid, qinfoReady->currentTime, qinfoReady->prio, qinfoReady->remainingTime, qinfoReady->arrivalTime);

				// ready queue안에 여러게 있을때 head를 free 지우고 그 다음 노드로 업데이트
				if (qinfoReady->next != NULL){
					queue *theTemp;
					theTemp = qinfoReady;
					qinfoReady = qinfoReady->next;
					theTemp->next = NULL;
					free(theTemp);
				}
				pthread_mutex_unlock(&mutex_update_tid);
			}
			blockedInfo[sem_id]->semid = sem_id;

			// 왜 시그널인지 아직 모르겠음
			
			pthread_mutex_lock(&mutexCond);
			if(option==0) pthread_cond_signal(&condFCFS);
			else pthread_cond_signal(&cond[qinfoRunning->currentid]);
			pthread_mutex_unlock(&mutexCond);
		}

		else if(option == MLFQ){
			updateBlockedMLFQ(blockedInfo[sem_id], qinfoRunningMLFQ->currentid, qinfoRunningMLFQ->currentTime, qinfoRunningMLFQ->prio, qinfoRunningMLFQ->remainingTime, qinfoRunningMLFQ->arrivalTime, qinfoRunningMLFQ->level, qinfoRunningMLFQ->age);
			// Ready Queue head -> Running Queue 이동
			for(int i = 0; i < 5; i++){
				// Ready Q 에 누가 있을때
				if(multiInfo[i]->currentid != 0){
					// Ready Q 에 한명만 있을때
					if(multiInfo[i]->next == NULL){
						updateMLFQ(qinfoRunningMLFQ, multiInfo[i]->currentid, multiInfo[i]->currentTime, multiInfo[i]->prio, multiInfo[i]->remainingTime, multiInfo[i]->arrivalTime, multiInfo[i]->level, multiInfo[i]->age);
						updateMLFQ(multiInfo[i], 0, 0, 0, 0, 0, 0, 0);
					}
					// Ready Q 에 여러명 있을때 맨앞애를 Running 으로 옮겨주고 전체 한칸씩 앞으로 땡기기
					else{
						multiQ *temp = multiInfo[i]->next;
						updateMLFQ(qinfoRunningMLFQ, multiInfo[i]->currentid, multiInfo[i]->currentTime, multiInfo[i]->prio, multiInfo[i]->remainingTime, multiInfo[i]->arrivalTime, multiInfo[i]->level, multiInfo[i]->age);
						updateMLFQ(multiInfo[i], temp->currentid, temp->currentTime, temp->prio, temp->remainingTime, temp->arrivalTime, temp->level, temp->age);
						multiInfo[i]->next = temp->next;
						temp->next = NULL;
						free(temp);
					}
					break;
				}
			}
			blockedInfo[sem_id]->semid = sem_id;
		}
	}

	return globalCurrentTime;
}

int V( float currentTime, int tid, int sem_id){ // returns current global time

	globalCurrentTime = currentTime;

	if(blockedInfo[sem_id]->currentid != 0){

		if(option == 1 || option == 2 || option == 3){
			// 다른애가 찾아왔는데 ready queue에 아무거도 없을때
			if (qinfoReady->currentid == 0){
				pthread_mutex_lock(&mutex_update_tid);
				updateQ(qinfoReady, blockedInfo[sem_id]->currentid, blockedInfo[sem_id]->currentTime, blockedInfo[sem_id]->prio, blockedInfo[sem_id]->remainingTime, blockedInfo[sem_id]->arrivalTime);
				pthread_mutex_unlock(&mutex_update_tid);
			}
			// 다른애가 찾아왔는데 ready queue에 누가 있을때
			else{
				queue *readyTail = qinfoReady; //check if tid is in ready queue

				// tail 로 point 하게끔 룹
				while(readyTail->next != NULL){
					// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
					if (readyTail->currentid == tid){
						break;
					}
					// 새로운 ready queue
					else{
						readyTail = readyTail->next;
					}
				}
				// INSERT: tail 에서 새로운 애가 들어왔을때 새로운 node 생성 (temp)
				if(readyTail->next == NULL && readyTail->currentid != tid){
					pthread_mutex_lock(&mutex_update_tid);
					queue *temp = malloc(sizeof(queue));
					updateQ(temp, blockedInfo[sem_id]->currentid, blockedInfo[sem_id]->currentTime, blockedInfo[sem_id]->prio, blockedInfo[sem_id]->remainingTime, blockedInfo[sem_id]->arrivalTime);
					temp->next = NULL;
					readyTail->next = temp;
					pthread_mutex_unlock(&mutex_update_tid);
				}
			}
			updateBQ(blockedInfo[sem_id], 0, 0, 0, 0, 0);
		}

		else if(option == MLFQ){
			if(multiInfo[blockedInfo[sem_id]->level]->currentid == 0){
				updateMLFQ(multiInfo[blockedInfo[sem_id]->level], blockedInfo[sem_id]->currentid, blockedInfo[sem_id]->currentTime, blockedInfo[sem_id]->prio, blockedInfo[sem_id]->remainingTime, blockedInfo[sem_id]->arrivalTime, blockedInfo[sem_id]->level, blockedInfo[sem_id]->age);
			}
			// 자기 레벨 Ready Q 에 누가 있을때
			else if(multiInfo[blockedInfo[sem_id]->level]->currentid != 0){
				
				//*************** 자기레벨 Ready Q 맨뒤에 붙이는거 ****************//
				// tail 로 point 하게끔 룹

				multiQ *readyTail; 
				readyTail = multiInfo[blockedInfo[sem_id]->level];
				while(readyTail->next != NULL){
					// 들어오는 tid 가 ready queue에 이미 있으면 break (중복되니깐)
					if (readyTail->currentid == blockedInfo[sem_id]->currentid){
						break;
					}
					// 새로운 ready queue
					else{
						readyTail = readyTail->next;
					}
				}
				if(readyTail->next == NULL && readyTail->currentid != blockedInfo[sem_id]->currentid){
					multiQ *temp = malloc(sizeof(multiQ));
					updateMLFQ(temp, blockedInfo[sem_id]->currentid, blockedInfo[sem_id]->currentTime, blockedInfo[sem_id]->prio, blockedInfo[sem_id]->remainingTime, blockedInfo[sem_id]->arrivalTime, blockedInfo[sem_id]->level, blockedInfo[sem_id]->age);
					temp->next = NULL;
					readyTail->next = temp;
				}
			}
		}
		blockedInfo[sem_id]->semid = 0;
	}
	else{
		blockedInfo[sem_id]->semCount++;
	}

	return globalCurrentTime;
}