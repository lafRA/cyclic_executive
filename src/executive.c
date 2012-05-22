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

typedef struct executive_data_ {
		pthread_t thread;				//rappresentazione di pthread del thread
		
		pthread_cond_t execute;			//indica quando l'executive può mettersi in esecuzione 
		
		unsigned char stop_request;		//flag per stoppare l'esecuzione dell'executive
		
		pthread_mutex_t mutex;			//mutex dummy per acquisire la variabile condizione execute e il flag stop_request
} executive_data_t;



//----------------DATA---------------------//

static task_data_t* tasks;				//da inizializzare nella funzione ??, e da distruggere nella funzione ??

static executive_data_t executive;		//executive

static server_data_t p_server;			//server per i task periodici
static server_data_t ap_server;		//server per i task aperiodici

//static pthread_t executive;			//rappresentazione di pthread dell'executive
//server_data_t sp_server;

static int ap_request_flag;					//flag di richiesta per il task aperiodico 
static task_data_t* tasks = 0;				//da inizializzare nella funzione ??, e da distruggere nella funzione ??
static server_data_t p_server;			//server per i task periodici
static server_data_t ap_server;		//server per i task aperiodici
static executive_data_t executive;				//rappresentazione di pthread dell'executive
//server_data_t sp_server;

//----------------PROTOTYPE-----------------//
void p_task_handler(void* arg);
void p_server_handler(void* arg);
void ap_server_handler(void* arg);
void executive_handler(void* arg);

//----------------FUNCTION------------------//
void task_init() {
// 	task_destroy();		//nel caso la task_init venga chiamata due volte di fila senza una task_destroy();
	
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
	//inizializzo i mutex e le condition variable
	pthread_mutex_init(&executive.mutex, NULL);
	pthread_cond_init(&executive.execute, NULL);
	sched_attr.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_attr_setschedparam(&th_attr, &sched_attr);
	executive.stop_request = 1;
	assert(pthread_create(&executive.thread, &th_attr, executive_handler, NULL));
}

void task_destroy() {
	///NOTE: utilizzo il valore di tasks per capire se le cose sono già inizilizzate: tasks == 0 |==> niente è ancora stato inizializzato
	if(tasks == 0) {
		//non c'è niente da distruggere dato che non c'è nulla di inizializzato
		return;
	}
	
	//fermo l'esecuzione dei vari thread associati ai task
	//e distruggo mutex e condition variable
	int i;
	for(i = 0; i < NUM_P_TASKS; --i ) {
		pthread_cancel(tasks[i].thread);			//fermo la sua esecuzione
		pthread_join(tasks[i].thread, NULL);		///FIXME: mettiamo una join per assicurarci che sia terminato prima di distruggere le sue strutture dati??
		pthread_mutex_destroy(&tasks[i].mutex);	//distruggo il mutex
		pthread_cond_destroy(&tasks[i].execute);	//distruggo la condition variable
	}
	
	free(tasks);
	tasks = 0;
	
	//fermo il server periodico
	//e distruggo le sue strutture dati
	pthread_canceal(p_server.thread);
	pthread_join(p_server.thread, NULL);
	pthread_mutex_destroy(&p_server.execute_mutex);
	pthread_mutex_destroy(&p_server.finish_mutex);
	pthread_cond_destroy(&p_server.execute);
	pthread_cond_destroy(&p_server.finish);
	
	//fermo il server aperiodico
	//e distruggo le sue strutture dati
	pthread_canceal(ap_server.thread);
	pthread_join(ap_server.thread, NULL);
	pthread_mutex_destroy(&ap_server.execute_mutex);
	pthread_mutex_destroy(&ap_server.finish_mutex);
	pthread_cond_destroy(&ap_server.execute);
	pthread_cond_destroy(&ap_server.finish);
	
	//fermo l'executive
	//e distruggo le sue strutture dati
	pthread_canceal(executive.thread);
	pthead_join(executive.thread, NULL);
	pthread_mutex_destroy(&executive.mutex);
	pthread_cond_destroy(&executive.execute);
	executive.stop_request = 1;
	
	return;
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

void executive_handler(void * arg) {
	//per prima cosa l'executive aspetta il via per l'esecuzione
	pthread_mutex_lock(&executive.mutex);
	pthread_cond_wait(&executive.execute, &executive.mutex);
	pthread_mutex_unlock(&executive.mutex);
	
	unsigned int frame_num;		//indice del frame corrente
	unsigned int threshold;		//soglia di sicurezza per lo slack stealing
	
	int i;	//indice del task corrente
	
	
	//inizializzazione:
	frame_num = 0;
	threshold = 1;		//TODO: valore a caso poi decidiamo un valore sensato
	i = 0;
	
	//loop forever
	while(!executive.stop_request) {
		
		if(ap_request_flag) {			//se c'è una richiesta per il task aperiodico
			//per prima cosa bisogna svegliare il server del task apediodico
			if(SLACK[frame_num] > threshold) {	//poi mettiamo un valore di soglia che tenga conto del tempo per mettere in esecuzione il server aperiodico
			
				//diciamo al server per quanto tempo al max può eseguire
				//FIXME  ap_server.time_limit = SLACK[frame_num] - threshold;
	
				pthread_mutex_lock(&ap_server.execute_mutex);
				pthread_cond_signal(&ap_server.execute);
				pthread_mutex_unlock(&ap_server.execute_mutex);
			
				//metto l'executive in attesa che finisca il server aperiodico:
				pthread_mutex_lock(&ap_server.finish_mutex);
				pthread_cond_wait(&ap_server.finish, &ap_server.finish_mutex);
				pthread_mutex_unlock(&ap_server.finish_mutex);
			}
		}
		
		
		//a questo punto abbiamo eseguito il task aperiodico, mettiamo in esecuzione il server dei task periodici:
		pthread_mutex_lock(&p_server.execute_mutex);
		pthread_cond_signal(&p_server.execute);
		pthread_mutex_unlock(&p_server.execute_mutex);
			
		//metto l'executive in attesa che finisca il server periodico:
		pthread_mutex_lock(&ap_server.finish_mutex);
		pthread_cond_wait(&ap_server.finish, &ap_server.finish_mutex);
		pthread_mutex_unlock(&ap_server.finish_mutex);
		
		
		frame_num++;
		
		if(frame_num == NUM_FRAMES)
			frame_num = 0;
	}
}
	
void p_server_handler(void* arg) {
}

void ap_server_handler(void* arg) {
}

//void executive_handler(/*...*/) {
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
//}