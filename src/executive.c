/* traccia dell'executive (pseudocodice) */
#ifdef MULTIPROC
	//sistema multiprocessore
	#define _GNU_SOURCE
#endif

#ifdef	DEBUG
	#define	PRINT(x, m) fprintf(stderr, "%s --> #m", (x));
	#define TRACE_D(x, m) fprintf(stderr, "%s --> #m = %d", (x), (m));
	#define TRACE_F(x, m) fprintf(stderr, "%s --> #m = %f", (x), (m));
	#define TRACE_C(x, m) fprintf(stderr, "%s --> #m = %c", (x), (m));
	#define TRACE_S(x, m) fprintf(stderr, "%s --> #m = %s", (x), (m));
#else
	#define PRINT(x, m)
	#define TRACE_D(x, m)
	#define TRACE_F(x, m)
	#define TRACE_C(x, m)
	#define TRACE_S(x, m)
#endif	//DEBUG

//file di configurazione esterno (di prova)
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <errno.h>


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

typedef struct executive_data_ {
		pthread_t thread;				//rappresentazione di pthread del thread
		
		pthread_cond_t execute;			//indica quando l'executive può mettersi in esecuzione 
		pthread_mutex_t mutex;			//mutex dummy per acquisire la variabile condizione execute e il flag stop_request
		
		unsigned char stop_request;		//flag per stoppare l'esecuzione dell'executive
		
} executive_data_t;

//----------------DATA---------------------//

static unsigned char ap_request_flag = 0;	//flag di richiesta per il task aperiodico 
static pthread_mutex_t ap_request_flag_mutex; 
static task_data_t ap_task;

static task_data_t* tasks = 0;				//da inizializzare nella funzione ??, e da distruggere nella funzione ??


static executive_data_t executive;				//rappresentazione di pthread dell'executive

//----------------PROTOTYPE-----------------//
void* executive_handler(void* arg);
void* p_task_handler(void* arg);
void* ap_task_handler(void* arg);

//----------------FUNCTION------------------//
void init() {
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
	
	
	//* CREO IL THREAD ASSOCIATO AL TASK PERIODICO
	pthread_mutex_init(&ap_request_flag_mutex, NULL);	//inizializzo il mutex associato al flag di richiesta del task aperiodico
	pthread_mutex_init(&ap_task.mutex, NULL);
	//inizializzo la condition variable
	pthread_cond_init(&ap_task.execute, NULL);
	ap_task.thread_id = -1;
	//inizilizzo lo stato del task aperiodico a TASK_COMPLETE
	ap_task.state = TASK_COMPLETE;
	//creo il thread con gli attributi desiderati
	assert(pthread_create(&ap_task.thread, &th_attr, ap_task_handler, (void*)(&ap_task)));
	
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

void destroy() {
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
	
	//fermo il task aperiodico
	pthread_cancel(ap_task.thread);			//fermo la sua esecuzione
	pthread_join(ap_task.thread, NULL);		///FIXME: mettiamo una join per assicurarci che sia terminato prima di distruggere le sue strutture dati??
	pthread_mutex_destroy(&ap_task.mutex);	//distruggo il mutex
	pthread_cond_destroy(&ap_task.execute);	//distruggo la condition variable
	
	//fermo l'executive
	//e distruggo le sue strutture dati
	pthread_cancel(executive.thread);
	pthread_join(executive.thread, NULL);
	pthread_mutex_destroy(&executive.mutex);
	pthread_cond_destroy(&executive.execute);
	executive.stop_request = 1;
	
	
	//fermo il task aperiodico
	pthread_cancel(ap_task.thread);			//fermo la sua esecuzione
	pthread_join(ap_task.thread, NULL);		///FIXME: mettiamo una join per assicurarci che sia terminato prima di distruggere le sue strutture dati??
	pthread_mutex_destroy(&ap_task.mutex);	//distruggo il mutex
	pthread_cond_destroy(&ap_task.execute);	//distruggo la condition variable
	
	//fermo l'executive
	//e distruggo le sue strutture dati
	pthread_cancel(executive.thread);
	pthread_join(executive.thread, NULL);
	pthread_mutex_destroy(&executive.mutex);
	pthread_cond_destroy(&executive.execute);
	executive.stop_request = 1;
	
	return;
}

void ap_task_request() {
	pthread_mutex_lock(&ap_request_flag_mutex);
		ap_request_flag = 1;
	pthread_mutex_unlock(&ap_request_flag_mutex);
}

/* Conta i task che devono essere eseguiti in un frame */
int count_task(int schedule[]) {
	int i;
	
	i = 0;
	while(schedule[i] = -1) {
		++i;
	}
	
	return i;
}


void* ap_task_handler(void* arg) {
	task_data_t* data = (task_data_t*) arg;
	
	while(1) {
		pthread_mutex_lock(&data->mutex);
		
		while(data->state != TASK_PENDING) {
			pthread_cond_wait(&data->execute, &data->mutex);	//aspetto fino a quando l'execute non mi segnala di eseguire
		}
		data->state = TASK_RUNNING;				//imposto il mio stato a RUNNING
		pthread_mutex_unlock(&data->mutex);
		
		//eseguo il codice utente
		(*AP_TASK)();
		
		
		pthread_mutex_lock(&executive.mutex);	//acquisisco il mutex dell'executive perchè non voglio essere interrotto tra l'operazione di aggiornamento stato e quella di signal all'executive.
			//aggiornamento dello stato e signal all'executive eseguite in modo atomico rispetto all'executive
			pthread_mutex_lock(&data->mutex);
			data->state = TASK_COMPLETE;				//imposto il mio stato a COMPLETE
			pthread_mutex_unlock(&data->mutex);
			
			//se fossi interrotto qui l'executive mi vedrebbe come completato, ma, alla prossima volta che viene schedulato il task aperiodico, invece di iniziare una nuova esecuzione l'unica cosa che fa è segnalare l'executive..andrebbe quindi persa un'intera esecuzione i di task aperiodico.
			//segnalo all'executive che ho completato
			pthread_cond_signal(&executive.execute);
		pthread_mutex_unlock(&executive.mutex);
	}
	
	return NULL;
}

void* p_task_handler(void* arg) {
	task_data_t* data = (task_data_t*) arg;
	
	while(1) {
		pthread_mutex_lock(&data->mutex);
		while(data->state != TASK_PENDING) {
			pthread_cond_wait(&data->execute, &data->mutex);	//aspetto fino a quando l'execute non mi segnala di eseguire
		}
		data->state = TASK_RUNNING;				//imposto il mio stato a RUNNING
		pthread_mutex_unlock(&data->mutex);
		
		(*P_TASKS[data->thread_id])();			//codice utente
		
		pthread_mutex_lock(&data->mutex);
		data->state = TASK_COMPLETE;			//metto il mio stato a COMPLETE
		pthread_mutex_unlock(&data->mutex);
	}
	
	return NULL;
}

void* executive_handler(void * arg) {
	///				PROLOGO				///
	//rendiamo l'executive cancellabile:
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);	//FIXME: controlla se non va bene
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	//per prima cosa l'executive aspetta il via per l'esecuzione
	pthread_mutex_lock(&executive.mutex);
	while(executive.stop_request) {
		pthread_cond_wait(&executive.execute, &executive.mutex);
	}
	pthread_mutex_unlock(&executive.mutex);
	
	//ora controllo che il numero di frame sia corretto, se il numero di frame non è un divisore della linghezza dell'iperperiodo allora la dimensione del frame è sbagliata ed è inutile continuare
	assert((H_PERIOD % NUM_FRAMES) != 0);
	
	///				DATI				///
	
	unsigned int frame_num;		//indice del frame corrente
	unsigned int threshold;		//soglia di sicurezza per lo slack stealing
	int frame_dim;				//dimensione del frame
	int i;						//indice al task corrente
	
	unsigned char timeout_expired;		//serve per sapere se è scaduto il timeout della fine del frame
	struct timespec time;
	struct timeval utime;
	
	struct sched_param th_param;		//per modificare la priorità dei thread
	
	unsigned char task_not_completed;	//servirà per controllare se i task del frame precedente hanno terminato
	int new_num_elements;				//servirà per costruire una nuova schedule
	int ind;							//servirà per costruire la nuova schedule
	
	///			INIZIALIZZAZIONE		///
	
	//inizializzazione delle variabili:
	frame_dim = H_PERIOD / NUM_FRAMES;
	frame_num = 0;
	threshold = 1;		//TODO: valore a caso poi decidiamo un valore sensato
	
	
	clock_gettime(CLOCK_REALTIME, &time);		//numero di secondi e microsecondi da EPOCH..............FIXME clock_gettime()
	time.tv_sec = utime.tv_sec;
	time.tv_nsec = utime.tv_usec * 1000;
	timeout_expired = 0;
	
	///			LOOP FOREVER			///
	
	while(1) {
		//controlliamo se posso procedere con l'esecuzione o se l'executive è in pausa:
		pthread_mutex_lock(&executive.mutex);	//per proteggere la variabile executive.stop_request
		while(executive.stop_request) {
			pthread_cond_wait(&executive.execute, &executive.mutex);
		}
		pthread_mutex_unlock(&executive.mutex);
		
		
		///			SCHEDULING DEI TASK APERIODICI			///
		
		//se c'è una richiesta di un task aperiodico e c'è abbastanza slack lo eseguo
		pthread_mutex_lock(&ap_request_flag_mutex);
		if(ap_task_request) {			//c'è una richiesta per un task aperiodico
			pthread_mutex_unlock(&ap_request_flag_mutex);
			
			pthread_mutex_lock(&ap_task.mutex);
			if(ap_task.state != TASK_COMPLETE) {		//c'è una richiesta ma l'istanza precedente non ha ancora terminato
				pthread_mutex_unlock(&ap_task.mutex);
				
				//segnalo la deadline miss
				fprintf(stderr, "DEADLINE MISS (APERIODIC TASK) \n");
				
				//se c'è abbastanza slack time metto in esecuzione il task aperiodico che non aveva terminato e salto la richiesta
				if(SLACK[frame_num] > threshold) {		
					//gli alzo la priorità
					th_param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
					pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
					
					//...e mi metto in attesa
					time.tv_sec += ( time.tv_nsec + 1000000000 ) / 1000000000;		//TODO: come timeout mettiamo lo slack-threshold se il task aperiodico finisce prima il server sveglia l'executive
					time.tv_nsec = ( time.tv_nsec + 1000000000 ) % 1000000000;
					
					pthread_mutex_lock(&ap_task.mutex);
					if(pthread_cond_timedwait(&ap_task.execute, &ap_task.mutex, &time) == ETIMEDOUT) {
						//se scade il timeout abbasso la priorità al task aperiodico così viene eseguito dopo tutti i task periodici, se c'è tempo
						th_param.sched_priority = (sched_get_priority_max(SCHED_FIFO) - 1) - count_task(SCHEDULE[frame_num]);
						pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
					}
					pthread_mutex_unlock(&ap_task.mutex);
				}
			}
			else {				//c'è una nuova richiesta e l'istanza precedente ha completato
				//pthread_mutex_unlock(&ap_task.mutex);
				
				//pthread_mutex_lock(&ap_task.mutex);
				ap_task.state = TASK_PENDING;
				pthread_cond_signal(&ap_task.execute);
				pthread_mutex_unlock(&ap_task.mutex);
				 
				time.tv_sec += ( time.tv_nsec + 1000000000 ) / 1000000000;		//TODO: come timeout mettiamo lo slack-threshold se il task aperiodico finisce prima il server sveglia l'executive
				time.tv_nsec = ( time.tv_nsec + 1000000000 ) % 1000000000;
				 
				//metto l'executive in attesa che finisca il task aperiodico oppure che finisca lo slack time a disposizione:
				pthread_mutex_lock(&ap_task.mutex);
				if(pthread_cond_timedwait(&ap_task.execute, &ap_task.mutex, &time) == ETIMEDOUT) {
					//se scade il timeout abbasso la priorità al task aperiodico così viene eseguito dopo tutti i task periodici, se c'è tempo
					th_param.sched_priority = (sched_get_priority_max(SCHED_FIFO) - 1) - count_task(SCHEDULE[frame_num]);
					pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
				}
				pthread_mutex_unlock(&ap_task.mutex);
			}
		}
		else {							//non c'è una nuova richiesta
			pthread_mutex_unlock(&ap_request_flag_mutex);
			
			pthread_mutex_lock(&ap_task.mutex);
			if(ap_task.state != TASK_COMPLETE) {		//se c'è un'istanza precedente che deve completare
				pthread_mutex_unlock(&ap_task.mutex);
				
				//continuo la sua esecuzione dandogli una priorità alta:
				th_param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
				pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
				
				//...e mi metto in attesa
				time.tv_sec += ( time.tv_nsec + 1000000000 ) / 1000000000;		//TODO: come timeout mettiamo lo slack-threshold se il task aperiodico finisce prima il server sveglia l'executive
				time.tv_nsec = ( time.tv_nsec + 1000000000 ) % 1000000000;
					
				pthread_mutex_lock(&ap_task.mutex);
				if(pthread_cond_timedwait(&ap_task.execute, &ap_task.mutex, &time) == ETIMEDOUT) {
					//se scade il timeout abbasso la priorità al task aperiodico così viene eseguito dopo tutti i task periodici, se c'è tempo
					th_param.sched_priority = (sched_get_priority_max(SCHED_FIFO) - 1) - count_task(SCHEDULE[frame_num]);
					pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
				}
				pthread_mutex_unlock(&ap_task.mutex);
			}
			else {
				pthread_mutex_lock(&ap_task.mutex);
				
				//bho!!
			}
		}
			
		///			SCHEDULING DEI TASK PERIODICI			///
			
		//verifico che i task del frame precedente abbiano finito l'esecuzione, se non hanno finito salto le esecuzioni successive e li faccio continuare:			come faccio????????
		ind = 0;
		task_not_completed = 0;
		while((!task_not_completed) && (ind < count_task(SCHEDULE[frame_num - 1]))) {
			pthread_mutex_lock(&tasks[SCHEDULE[frame_num - 1][ind]].mutex);
			if(tasks[SCHEDULE[frame_num - 1][ind]].state != TASK_COMPLETE) {	//un task del frame precedente non ha terminato
				task_not_completed = 1;
			}
			pthread_mutex_unlock(&tasks[SCHEDULE[frame_num - 1][ind]].mutex);
			++ind;
		}
		
		if(task_not_completed) {
			//rimpiazzo la schedule corrente (che dovrei eseguire in questo frame) con una nuova schedule che contiene i task del frame precedente che non hanno ancora eseguito
			--ind;	//mi devo portare l'indice indietro...TODO:ottimizzarlo
			
			//ora mi costruisco la schedule nuova:
			new_num_elements = count_task(SCHEDULE[frame_num - 1]) - ind + 1;			//numero di task che andranno a far parte della nuova schedule
			
			//ora dovrei andarli a sostituire in SCHEDULE[frame_num]
			free(SCHEDULE[frame_num]);
			SCHEDULE[frame_num] = (int *) malloc( sizeof( int ) * (new_num_elements + 1) );
			
			//ora vado a sostutuire
			for(i = 0; i < new_num_elements; ++i) {
				SCHEDULE[frame_num][i] = SCHEDULE[frame_num - 1][ind];
				++ind;
			}
			SCHEDULE[frame_num][new_num_elements] = -1;
		}
		
		//mettiamo in esecuzione i task:
		for(i = 0; i < count_task(SCHEDULE[frame_num]); ++i) {
			//assegnamo le priorità
			th_param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1 - i;
			pthread_setschedparam(tasks[SCHEDULE[frame_num][i]].thread, SCHED_FIFO, &th_param);
					
			//mettiamo lo stato del task a PENDING
			pthread_mutex_lock(&tasks[SCHEDULE[frame_num][i]].mutex);		//per proteggere lo stato e la variabile condizione del task	
			tasks[SCHEDULE[frame_num][i]].state = TASK_PENDING;
			pthread_cond_signal(&tasks[SCHEDULE[frame_num][i]].execute);
			pthread_mutex_unlock(&tasks[SCHEDULE[frame_num][i]].mutex);		//per proteggere lo stato e la variabile condizione del task
		}
			
		time.tv_sec += ( time.tv_nsec + 1000000000 ) / 1000000000;			//FIXME: impostare il tempo
		time.tv_nsec = ( time.tv_nsec + 1000000000 ) % 1000000000;
			
		//mi metto in attesa che finisca il tempo del frame:
		pthread_mutex_lock(&executive.mutex);
		while(pthread_cond_timedwait(&executive.execute, &executive.mutex, &time) != ETIMEDOUT) ; 
		pthread_mutex_unlock(&executive.mutex);
			
		//non controllo se c'è una richiesta del task aperiodico perchè la mando al frame successivo
			
		frame_num++;
			
		if(frame_num == NUM_FRAMES) {
			frame_num = 0;
		}
			
	}
	return NULL;
}

int main(int argc, char** argv) {
	init();
	pthread_join(executive.thread, NULL);
	destroy();
}
