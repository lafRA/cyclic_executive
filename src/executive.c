/* traccia dell'executive (pseudocodice) */
#include "executive-config.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

//----------------TYPES---------------------//

typedef enum {
	TASK_RUNNING,	//il job del task è in esecuzione
	TASK_COMPLETE,	//il job ha terminato, attende una nuova esecuzione
	TASK_PENDING	//il job è pronto per eseguire, ma non ha ancora iniziato
} task_state_t;

typedef struct task_data_ {
	unsigned int thread_id;		//ID del task: identifica l'indice del puntatore a funzione nell'array P_TASKS
	task_state_t state;			//indica lo stato del job corrente del task
	
	pthread_t thread;			//rappresentazione di pthread del thread
	pthread_mutex_t mutex;		//mutex per regolare l'accesso allo stato del task
	pthread_cond_t execute;		//condition variable che segnala quando il job del task può eseguire
} task_data_t;

typedef struct server_data_ {
	pthread_t thread;		//rappresentazione di pthread del thread
	
	pthread_cond_t execute;			//indica quando il server può mettersi in esecuzione
	pthread_mutex_t execute_mutex;	//mutex dummy per acquisire la variabile condizione
	
	pthread_cond_t finish;			//indica all'executive (che, dopo aver segnalato di partire su 'execute' si mette in attesa su questa variabile) che il server ha terminato
	pthread_mutex_t finish_mutex;	//mutex dummy per acquisire la variabile condizione
	
	struct timeval time_limit;		//tempo massimo entro cui l'esecuzione del server deve terminare
} server_data_t;

//----------------DATA---------------------//

static task_data_t* tasks;				//da inizializzare nella funzione ??, e da distruggere nella funzione ??

static server_data_t p_server;			//server per i task periodici
static server_data_t ap_server;		//server per i task aperiodici

static pthread_t executive;				//rappresentazione di pthread dell'executive
//server_data_t sp_server;

//----------------FUNCTION------------------//

void ap_task_request() {
// 	...
}

void p_task_handler(/*...*/) {
// 	...
}

void ap_task_handler(/*...*/) {
// 	...
}

void executive_handler(/*...*/) {
// 	struct timespec time;
// 	struct timeval utime;
// 
// 	gettimeofday(&utime,NULL);
// 
// 	time.tv_sec = utime.tv_sec;
// 	time.tv_nsec = utime.tv_usec * 1000;
// 
// 	while(...) {
// 		...
// 
// 		time.tv_sec += ( time.tv_nsec + nanosec ) / 1000000000;
// 		time.tv_nsec = ( time.tv_nsec + nanosec ) % 1000000000;
// 		...
// 		pthread_cond_timedwait( ..., &time );
// 		...
// 	}
}
