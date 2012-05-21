/* traccia dell'executive (pseudocodice) */
#ifdef MULTIPROC
	//sistema multiprocessore
	#define _GNU_SOURCE
#endif

//file di configurazione esterno (di prova)
#include "executive-config.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>

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
static task_data_t* tasks = 0;				//da inizializzare nella funzione ??, e da distruggere nella funzione ??
static server_data_t p_server;			//server per i task periodici
static server_data_t ap_server;		//server per i task aperiodici
static executive_data_t executive;				//rappresentazione di pthread dell'executive
//server_data_t sp_server;

//----------------PROTOTYPE-----------------//
void p_task_handler(void* arg);
void p_server_handler(void* arg);
void ap_server_handler(void* arg);

//----------------FUNCTION------------------//
void task_init() {
	//* CREO UN POOL DI THREAD PER GESTIRE I VARI TASK ED ASSOCIO AD OGNUNO LA PROPRIA STRUTTURA DATI
	//creazione dell'array di N_P_TASKS elementi del tipo task_data_t
	tasks = calloc(NUM_P_TASKS, sizeof(task_data_t));
	assert(tasks != NULL);
	//preparo le proprietà dei thread associati ai task periodici
	pthread_attr_t th_attr;
	pthread_attr_init(&th_attr);
	pthread_attr_setinheritsched(&th_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&th_attr, SCHED_FIFO);
	
#ifdef MULTIPROC
	//anche su un sistema monoprocessore voglio che i task siano schedulati su un singolo core
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	//affinità al processore 0
	pthread_attr_setaffinity(&th_attr, sizeof(cpu_set_t), &cpuset);
#endif
	
	//priorità dei thread: di default la pongo al minimo realtime
	struct sched_param sched_attr;
	sched_attr.sched_priority = sched_get_priority_min(SCHED_FIFO);
	pthread_attr_setschedparam(&th_attr, &sched_attr);
	
	int i;
	for(i = 0; i < NUM_P_TASKS; ++i) {
		//inizializzo il mutex
		pthread_mutex_init(&tasks[i].mutex, NULL);
		//inizializzo la condition variable
		pthread_cond_init(&tasks[i].execute, NULL);
		//inizializzo l'ID del thread come l'indice con cui lo creo
		tasks[i].thread_id = i;
		//inizilizzo lo stato del task a TASK_COMPLETE
		///NOTE: in questo caso posso accedere allo stato senza il mutex perchè, a questo punto di inizializzazione, non possono esistere altri processi che cercano di accedervi.
		tasks[i].state = TASK_COMPLETE;
		//creo il thread con gli attributi desiderati
		assert(pthread_create(&tasks[i].thread, &th_attr, p_task_handler, (void*)(tasks+i)));
	}
	
	//*	CREO IL SERVER DEI TASK PERIODICI
	//inizializzo i mutex e le condition variable
	pthread_mutex_init(&p_server.execute_mutex, NULL);
	pthread_cond_init(&p_server.execute, NULL);
	pthread_mutex_init(&p_server.finish_mutex, NULL);
	pthread_cond_init(&p_server.finish, NULL);
	//di default questi server hanno priorità pari a quella massima - 1;
	sched_attr.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	pthread_attr_setschedparam(&th_attr, &sched_attr);
	assert(pthread_create(&p_server.thread, &th_attr, p_server_handler, NULL));
	
	//*	CREO IL SERVER DEI TASK APERIODICI
	//inizializzo i mutex e le condition variable
	pthread_mutex_init(&ap_server.execute_mutex, NULL);
	pthread_cond_init(&ap_server.execute, NULL);
	pthread_mutex_init(&ap_server.finish_mutex, NULL);
	pthread_cond_init(&ap_server.finish, NULL);
	//di default questi server hanno priorità pari a quella massima - 1;
	assert(pthread_create(&ap_server.thread, &th_attr, ap_server_handler, NULL));
	
	//*	CREO L'EXECUTIVE
	//l'executive detiene sempre la priorità massima
	///TODO: devo inizilizzare il mutex e la condition dell'executive
	sched_attr.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_attr_setschedparam(&th_attr, &sched_attr);
	assert(pthread_create(&executive.thread, &th_attr, executive_handler, NULL));
}

void task_destroy() {
	///NOTE: utilizzo il valore di tasks per capire se le cose sono già inizilizzate: tasks == 0 |==> niente è ancora stato inizializzato
	
	
}

void ap_task_request() {
// 	...
}

void p_task_handler(void* arg) {
// 	...
}

void ap_task_handler(void* arg) {
// 	...
}

void p_server_handler(void* arg) {
}

void ap_server_handler(void* arg) {
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
