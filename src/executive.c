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

typedef struct executive_data_ {
		pthread_t thread;				//rappresentazione di pthread del thread
		
		pthread_cond_t execute;			//indica quando l'executive può mettersi in esecuzione 
		
		unsigned char stop_request;		//flag per stoppare l'esecuzione dell'executive
		
		pthread_mutex_t mutex;			//mutex dummy per acquisire la variabile condizione execute e il flag stop_request
} executive_data_t;



//----------------DATA---------------------//

static task_data_t* tasks;				//da inizializzare nella funzione ??, e da distruggere nella funzione ??

static executive_data_t executive		//executive

static server_data_t p_server;			//server per i task periodici
static server_data_t ap_server;		//server per i task aperiodici

//static pthread_t executive;			//rappresentazione di pthread dell'executive
//server_data_t sp_server;

static ap_request_flag;					//flag di richiesta per il task aperiodico 

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

void executive_handler(void * arg) {
	//per prima cosa l'executive aspetta il via per l'esecuzione
	pthread_mutex_lock(&executive.mutex);
	pthread_cond_wait(&executive.execute, &executive.mutex);
	pthread_mutex_unlock(&executive_data.mutex);
	
	unsigned int frame_num;		//indice del frame corrente
	unsigned int threshold;		//soglia di sicurezza per lo slack stealing
	
	int i;	//indice del task corrente
	
	
	//inizializzazione:
	frame_num = 0;
	threshold = 1;		//TODO: valore a caso poi decidiamo un valore sensato
	i = 0;
	
	//loop forever
	while(!stop_request) {
		
		if(ap_request_flag) {			//se c'è una richiesta per il task aperiodico
			//per prima cosa bisogna svegliare il server del task apediodico
			if(SLACK[frame_num] > threshold) {	//poi mettiamo un valore di soglia che tenga conto del tempo per mettere in esecuzione il server aperiodico
			
				//diciamo al server per quanto tempo al max può eseguire
				ap_server.time_limit = SLACK[frame_num] - threshold;
	
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
