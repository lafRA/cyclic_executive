/* traccia dell'executive (pseudocodice) */
#ifdef MULTIPROC
	//sistema multiprocessore
	#define _GNU_SOURCE
#endif

//file di configurazione esterno (di prova)
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <errno.h>

//----------------DEFINES-------------------//
#define TIME_UNIT_NS 1e7
#ifndef	NDEBUG
	#define	PRINT(x, m) fprintf(stderr, ">>\t%s --> %s\n", (x), (m));
	#define TRACE_D(x, m) fprintf(stderr, ">>\t%s --> "#m" = %d\n", (x), (m));
	#define TRACE_L(x, m) fprintf(stderr, ">>\t%s --> "#m" = %ld\n", (x), (m));
	#define TRACE_LL(x, m) fprintf(stderr, ">>\t%s --> "#m" = %lld\n", (x), (m));
	#define TRACE_F(x, m) fprintf(stderr, ">>\t%s --> "#m" = %f\n", (x), (m));
	#define TRACE_C(x, m) fprintf(stderr, ">>\t%s --> "#m" = %c\n", (x), (m));
	#define TRACE_S(x, m) fprintf(stderr, ">>\t%s --> "#m" = %s\n", (x), (m));
#else
	#define PRINT(x, m)
	#define TRACE_D(x, m)
	#define TRACE_L(x, m)
	#define TRACE_LL(x, m)
	#define TRACE_F(x, m)
	#define TRACE_C(x, m)
	#define TRACE_S(x, m)
#endif	//NDEBUG

//----------------TYPES---------------------//
typedef enum {
	TASK_RUNNING,	//il job del task è in esecuzione
	TASK_COMPLETE,	//il job ha terminato, attende una nuova esecuzione
	TASK_PENDING	//il job è pronto per eseguire, ma non ha ancora iniziato
} task_state_t;

typedef struct task_data_ {
	unsigned int thread_id;		//ID del task: identifica l'indice del puntatore a funzione nell'array P_TASKS
	task_state_t state;			//indica lo stato del job corrente del task
	unsigned char init;			//flag che indica se il thread è stato correttamente inizializzato
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
struct timespec zero_time;

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
	pthread_attr_setaffinity_np(&th_attr, sizeof(cpu_set_t), &cpuset);
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
		tasks[i].init = 0;
		//inizilizzo lo stato del task a TASK_COMPLETE
		///NOTE: in questo caso posso accedere allo stato senza il mutex perchè, a questo punto di inizializzazione, non possono esistere altri processi che cercano di accedervi.
		tasks[i].state = TASK_COMPLETE;
		
		//la priorità crescente garantisce che tutti vengano effettivamente inizializzati
		sched_attr.sched_priority = sched_get_priority_min(SCHED_FIFO)+i;
		
		pthread_attr_setschedparam(&th_attr, &sched_attr);
		//creo il thread con gli attributi desiderati
		assert(pthread_create(&tasks[i].thread, &th_attr, p_task_handler, (void*)(tasks+i)) == 0);
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
	assert(pthread_create(&ap_task.thread, &th_attr, ap_task_handler, (void*)(&ap_task)) == 0);
	
	//*	CREO L'EXECUTIVE
	//l'executive detiene sempre la priorità massima
	//inizializzo i mutex e le condition variable
	pthread_mutex_init(&executive.mutex, NULL);
	pthread_cond_init(&executive.execute, NULL);
	sched_attr.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_attr_setschedparam(&th_attr, &sched_attr);
	executive.stop_request = 1;
	assert(pthread_create(&executive.thread, &th_attr, executive_handler, NULL) == 0);
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
	while(schedule[i] != -1) {
		++i;
	}
	
	return i;
}


void* ap_task_handler(void* arg) {
	task_data_t* data = (task_data_t*) arg;
	
	TRACE_D("ap_task_handler", data->thread_id)
	
	while(1) {
		pthread_mutex_lock(&data->mutex);
		
		while(data->state != TASK_PENDING) {
			PRINT("ap_task_handler", "waiting on 'execute'")
			pthread_cond_wait(&data->execute, &data->mutex);	//aspetto fino a quando l'execute non mi segnala di eseguire
		}
		PRINT("ap_task_handler", "wake up, setting state to TASK_RUNNING")
		data->state = TASK_RUNNING;				//imposto il mio stato a RUNNING
		pthread_mutex_unlock(&data->mutex);
		
		//eseguo il codice utente
		PRINT("ap_task_handler", "executing task code")
		(*AP_TASK)();
		PRINT("ap_task_handler", "task code complete")
		
		
		pthread_mutex_lock(&executive.mutex);	//acquisisco il mutex dell'executive perchè non voglio essere interrotto tra l'operazione di aggiornamento stato e quella di signal all'executive.
			//aggiornamento dello stato e signal all'executive eseguite in modo atomico rispetto all'executive
			pthread_mutex_lock(&data->mutex);
			PRINT("p_task_handler", "setting state to TASK_COMPLETE")
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
	
	TRACE_D("p_task_handler", data->thread_id)
	
	while(1) {
		pthread_mutex_lock(&data->mutex);
		while(data->state != TASK_PENDING) {
			if(data->init == 0) {
				data->init = 1;
				pthread_cond_signal(&data->execute);		//segnalo all'init() che sono stato creato
			}
			TRACE_D("p_task_handler::waiting on 'execute'", data->thread_id)
			pthread_cond_wait(&data->execute, &data->mutex);	//aspetto fino a quando l'execute non mi segnala di eseguire
		}
		TRACE_D("p_task_handler::wake up, setting state to TASK_RUNNING", data->thread_id)
		data->state = TASK_RUNNING;				//imposto il mio stato a RUNNING
		pthread_mutex_unlock(&data->mutex);
		
		TRACE_D("p_task_handler::executing task code", data->thread_id)
		(*P_TASKS[data->thread_id])();			//codice utente
		
		TRACE_D("p_task_handler::task code complete", data->thread_id)
		
		pthread_mutex_lock(&data->mutex);
		TRACE_D("p_task_handler::setting state tu TASK_COMPLETE", data->thread_id)
		data->state = TASK_COMPLETE;			//metto il mio stato a COMPLETE
		pthread_mutex_unlock(&data->mutex);
	}
	
	return NULL;
}

void print_deadline_miss(int index, unsigned long long absolute_frame_num) {
	struct timespec t;
	int hyperperiod;
	
	hyperperiod = absolute_frame_num / NUM_FRAMES;
	
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	
	if(index == -1) {
		fprintf(stderr, "** DEADLINE MISS (APERIODIC TASK) @ (%ld)s (%ld)ns from start\n\tframe %lld @ hyperperiod %d.\n", t.tv_sec, t.tv_nsec, absolute_frame_num % NUM_FRAMES, hyperperiod);
	} else  {
		fprintf(stderr, "** DEADLINE MISS (PERIODIC TASK %d) @ (%ld)s (%ld)ns from start\n\tframe %lld @ hyperperiod %d.\n", index, t.tv_sec, t.tv_nsec, absolute_frame_num % NUM_FRAMES, hyperperiod);
	}
	
}

void* executive_handler(void * arg) {
	PRINT("*********************************************************************", "executive started!!")
	
	///				PROLOGO				///
	//rendiamo l'executive cancellabile:
	/*pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	//per prima cosa l'executive aspetta il via per l'esecuzione
	pthread_mutex_lock(&executive.mutex);
	while(executive.stop_request) {
		pthread_cond_wait(&executive.execute, &executive.mutex);
	}
	pthread_mutex_unlock(&executive.mutex);*/
	
	//ora controllo che il numero di frame sia corretto, se il numero di frame non è un divisore della linghezza dell'iperperiodo allora la dimensione del frame è sbagliata ed è inutile continuare
	assert((H_PERIOD % NUM_FRAMES) == 0);
	
	///				DATI				///
	
	unsigned int frame_ind;				//indice del frame corrente
	unsigned int threshold;				//soglia di sicurezza per lo slack stealing
	int frame_dim;						//dimensione del frame
	unsigned long long frame_count;		//contatore incrementale
	int i;								//indice al task corrente
	
	//unsigned char timeout_expired;		//serve per sapere se è scaduto il timeout della fine del frame
	struct timespec time;
	struct timeval utime;
	
	struct sched_param th_param;		//per modificare la priorità dei thread
	
	unsigned char task_not_completed;	//servirà per controllare se i task del frame precedente hanno terminato
	int new_num_elements;				//servirà per costruire una nuova schedule
	int ind;							//servirà per costruire la nuova schedule
	
	///			INIZIALIZZAZIONE		///
	
	//inizializzazione delle variabili:
	frame_dim = H_PERIOD / NUM_FRAMES;
	frame_ind = 0;
	frame_count = 0;
	threshold = 1000000;		//TODO: valore a caso poi decidiamo un valore sensato
	
	TRACE_D("executive::inizializzazione", frame_dim)
	
	
// 	clock_gettime(CLOCK_REALTIME, &time);			//numero di secondi e microsecondi da EPOCH..............FIXME clock_gettime()
	clock_gettime(CLOCK_REALTIME, &zero_time);
	TRACE_L("executive::inizializzazione", zero_time.tv_sec)
	TRACE_L("executive::inizializzazione", zero_time.tv_nsec)
// 	time.tv_sec = utime.tv_sec;
// 	time.tv_nsec = utime.tv_usec * 1000;
// 	timeout_expired = 0;
	
	for(i = 0; i < NUM_P_TASKS; ++i) {
		pthread_setschedprio(tasks[i].thread, sched_get_priority_min(SCHED_FIFO));
	}
	
	///			LOOP FOREVER			///
	
	while(1) {
		PRINT("========================================================================", "new frame")
		TRACE_LL("executive::starting loop", frame_count)
		TRACE_D("executive::starting loop", frame_ind)
		
	#ifndef	NDEBUG
		{
			clock_gettime(CLOCK_REALTIME, &time);
			TIME_DIFF(zero_time, time)
			TRACE_L("executive_handler::wake up @", time.tv_sec)
			TRACE_L("executive_handler::wake up @", time.tv_nsec)
		}
#endif	//NDEBUG
		
		//controlliamo se posso procedere con l'esecuzione o se l'executive è in pausa:
		/*pthread_mutex_lock(&executive.mutex);	//per proteggere la variabile executive.stop_request
		while(executive.stop_request) {
			pthread_cond_wait(&executive.execute, &executive.mutex);
		}
		pthread_mutex_unlock(&executive.mutex);*/
		
		
		///			SCHEDULING DEI TASK APERIODICI			///
		//queste variabili mi servono per fare una copia delle variabili protette da mutex che dovrei testare negli if...mi faccio una copia così libero il mutex subito 
		unsigned char ap_request_flag_local;
		task_state_t ap_task_state_local;
		unsigned int frame_prec;
		
		//mi faccio le copie
		pthread_mutex_lock(&ap_request_flag_mutex);
		ap_request_flag_local = ap_request_flag;
		pthread_mutex_unlock(&ap_request_flag_mutex);
		
		pthread_mutex_lock(&ap_task.mutex);
		ap_task_state_local = ap_task.state;
		pthread_mutex_unlock(&ap_task.mutex);
		
		//se c'è una richiesta di un task aperiodico e c'è abbastanza slack lo eseguo
		if((ap_request_flag_local == 1) || (ap_task_state_local == TASK_RUNNING)) {
			TRACE_D("executive::aperiodic test", (ap_request_flag_local == 1) || (ap_task_state_local == TASK_RUNNING))
			if((ap_request_flag_local == 1) && (ap_task_state_local == TASK_RUNNING)) {
				TRACE_D("executive::aperiodic test", (ap_request_flag_local == 1) && (ap_task_state_local == TASK_RUNNING))
				//segnalo la deadline miss
// 				struct timespec t;
// 				clock_gettime(CLOCK_REALTIME, &t);
// 				TIME_DIFF(zero_time, t)
// 				fprintf(stderr, "** DEADLINE MISS (APERIODIC TASK) @ (%d)s (%d)ns from start.\n", t.tv_sec, t.tv_nsec);	///@fra ho aggiunto qualche info temporale
				print_deadline_miss(-1, frame_count);
			}
			
			//slack stealing
			if(SLACK[frame_ind] > 0) {
				PRINT("executive", "slack stealing")
				//se c'è stata una richiesta e nessun task aperiodico è in esecuzione devo abbassarla perchè inizio a servirla)
				//se c'è stata una richiesta e il task aperiodico era in esecuzione devo abbassarla perchè ha generato una deadline miss
				//se non c'è stata nessuna richiesta e il task era già in esecuzione questa è già bassa
				if(ap_request_flag_local == 1) {
					pthread_mutex_lock(&ap_request_flag_mutex);
					ap_request_flag = 0;
					pthread_mutex_unlock(&ap_request_flag_mutex);		///@fra ho aggiunto il reset del flag di richiesta.. secondo te è giusto così??
				}
				
				//gli alzo la priorità
				th_param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
				pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
				
				//...e mi metto in attesa
				time.tv_sec = zero_time.tv_sec;
				time.tv_nsec = zero_time.tv_nsec;		//inizializzo il tempo a zero_time
				
				//sommo la quantità di nanosecondi che passa tra lo zero_time e l'inizio di questo frame (contando che frame_count è già stato incrementato)
				//TIME_UNIT_NS = 10^7 = dimensione del quanto temporale espressa in nanosecondi
				time.tv_nsec += (TIME_UNIT_NS * frame_dim) * frame_count + (SLACK[frame_ind])*TIME_UNIT_NS - threshold;
		
				//normalizzo la struttura per riportarla in uno stato consistente
				time.tv_sec += ( time.tv_nsec ) / 1000000000;		//TODO: come timeout mettiamo lo slack-threshold se il task aperiodico finisce prima il server sveglia l'executive
				time.tv_nsec = ( time.tv_nsec ) % 1000000000;
				
#ifndef	NDEBUG
				{
					struct timespec time_rel;
					time_rel.tv_sec = time.tv_sec;
					time_rel.tv_nsec = time.tv_nsec;
					TIME_DIFF(zero_time, time_rel)
					TRACE_L("executive_handler::waiting for slack time", time_rel.tv_sec)
					TRACE_L("executive_handler::waiting for slack time", time_rel.tv_nsec)
				}
#endif	//NDEBUG
				
				pthread_mutex_lock(&ap_task.mutex);
				if(pthread_cond_timedwait(&ap_task.execute, &ap_task.mutex, &time) == ETIMEDOUT) {
					//se scade il timeout abbasso la priorità al task aperiodico così viene eseguito dopo tutti i task periodici, se c'è tempo
					/*th_param.sched_priority = (sched_get_priority_max(SCHED_FIFO) - 1) - count_task(SCHEDULE[frame_ind]);*/	//devi mettere sua priorità al minimo consentito perchè, metti che l'ap_task è ancora RUNNING, ma sono in un frame che non ha slack time. Dato k la sua priorità rimane quella impostata al fram prima mi trovo in una situazione in cui un task periodico e quello aperiodico hanno la stessa priorità. dato k lo scheduling è FIFO inoltre viene schedulato quello aperiodico perchè è in coda da più tempo!!!
// 					th_param.sched_priority = sched_get_priority_min(SCHED_FIFO) + 1;
// 					pthread_setschedparam(ap_task.thread, SCHED_FIFO, &th_param);
					pthread_setschedprio(ap_task.thread, sched_get_priority_min(SCHED_FIFO) + 1);
				} //else { il task ap ha completato, c'è qualche controllo da fare per garantire l'integrità della cosa???
				pthread_mutex_unlock(&ap_task.mutex);
			}
		}	///@fra mancava questa parentesi, ci vuole, vero??
			
		///			SCHEDULING DEI TASK PERIODICI			///

		//verifico che i task del frame precedente abbiano finito l'esecuzione, se non hanno finito salto le esecuzioni successive e li faccio continuare:			come faccio????????
// 		ind = 0;
// 		task_not_completed = 0;
// 		while((!task_not_completed) && (ind < count_task(SCHEDULE[frame_ind - 1]))) {		//fare variabile temp per count_task
// 			//TEST pthread_mutex_lock(&tasks[SCHEDULE[frame_ind - 1][ind]].mutex);
// 			if(tasks[SCHEDULE[frame_ind - 1][ind]].state != TASK_COMPLETE) {	//un task del frame precedente non ha terminato
// 				task_not_completed = 1;
// 			}
// 			//TEST pthread_mutex_unlock(&tasks[SCHEDULE[frame_ind - 1][ind]].mutex);
// 			++ind;
// 		}

#ifndef	NDEBUG
		clock_gettime(CLOCK_REALTIME, &time);
		TIME_DIFF(zero_time, time)
		TRACE_L("executive::serving periodic tasks", time.tv_sec)
		TRACE_L("executive::serving periodic tasks", time.tv_nsec)
#endif

		PRINT("executive","checking for late jobs")
		
		ind = 0;
		frame_prec = (frame_ind + NUM_FRAMES - 1) % NUM_FRAMES;	//indice del frame precedente
		task_not_completed = 0;
		int dim = count_task(SCHEDULE[frame_prec]);
		for(i = 0; i < dim; ++i) {
			if(task_not_completed) {
				/// @fra NOTE: Cmake, quando non in modalità debug, definisce automaticamente NDEBUG
#ifndef	NDEBUG
				assert(tasks[SCHEDULE[frame_prec][i]].state == TASK_PENDING);
#endif
				///TEST: provo a scrivere senza acquisire il mutex, tanto l'executive è SEMPRE quello a priorità più elevata
				print_deadline_miss(i, frame_count);
				tasks[SCHEDULE[frame_prec][i]].state = TASK_COMPLETE;
			} else {
				task_not_completed = (tasks[SCHEDULE[frame_prec][i]].state == TASK_RUNNING);
				if(task_not_completed) {
					print_deadline_miss(i, frame_count);
					pthread_setschedprio(tasks[SCHEDULE[frame_prec][i]].thread, sched_get_priority_min(SCHED_FIFO));
// 					ind = i;		//indice del task che è stato trovato ancora RUNNING all'inizio del frame
				}
			}
			///FIXME: è necessario????
			//abbasso la priorità di tutti i thread
			pthread_setschedprio(tasks[i].thread, sched_get_priority_min(SCHED_FIFO));
		}
		
// 		if(task_not_completed) {
// 			//rimpiazzo la schedule corrente (che dovrei eseguire in questo frame) con una nuova schedule che contiene i task del frame precedente che non hanno ancora eseguito
// 			--ind;	//mi devo portare l'indice indietro...TODO:ottimizzarlo
// 			
// 			//ora mi costruisco la schedule nuova:
// 			new_num_elements = count_task(SCHEDULE[frame_ind - 1]) - ind + 1;			//numero di task che andranno a far parte della nuova schedule
// 			
// 			//ora dovrei andarli a sostituire in SCHEDULE[frame_ind]
// 			free(SCHEDULE[frame_ind]);		/// @fra FIXME: WTF???? non si tocca la schedule dell'utente!!! O__o
// 			SCHEDULE[frame_ind] = (int *) malloc( sizeof( int ) * (new_num_elements + 1) );
// 			
// 			//ora vado a sostutuire
// 			for(i = 0; i < new_num_elements; ++i) {
// 				SCHEDULE[frame_ind][i] = SCHEDULE[frame_ind - 1][ind];
// 				++ind;
// 			}
// 			SCHEDULE[frame_ind][new_num_elements] = -1;
// 		}
		
		//mettiamo in esecuzione i task:
		/*
		 * Abbiamo adottato una politica che penalizza i task trovati in ritardo.
		 * I task che non hanno completato nel frame corrente (ma che hanno già iniziato l'esecuzione e non possono essere fermati)
		 * vengono schedulati solo se presenti nella schedule corrente, ma con la priorità più bassa tra quelli del frame.
		 * In questo modo se ci sono errori di programmazione che bloccano indefinitivamente il task, questo non compromette interamente la schedule.
		 * Dato che in generale possono esserci N task in ritardo, ognuno di questi lascia (dopo di sì) un buco nella scala di priorità, per questo
		 * la priorità dei task "corretti" viene alzata di rit, mentre tutti i task in ritardo guadagnano una priorità a partire da quella più bassa a salire, in modo da essere comunque schedulati per ultimi (con ordine arbitrario).
		 */
		dim = count_task(SCHEDULE[frame_ind]);
		TRACE_D("executive:: scheduling periodic tasks", dim)
		int rit = 0;	//numero di task in ritardo trovati per la schedule corrente
		for(i = 0; i < dim; ++i) {
			//assegnamo le priorità
			//TEST th_param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1 - i;
			//TEST pthread_setschedparam(tasks[SCHEDULE[frame_ind][i]].thread, SCHED_FIFO, &th_param);

			TRACE_D("executive::scheduling periodic tasks", SCHEDULE[frame_ind][i])
			
			///TEST: pthread_mutex_lock(&tasks[SCHEDULE[frame_ind][i]].mutex);		//per proteggere lo stato e la variabile condizione del task	
			if(tasks[SCHEDULE[frame_ind][i]].state == TASK_COMPLETE) {
				//mettiamo lo stato del task a PENDING
				tasks[SCHEDULE[frame_ind][i]].state = TASK_PENDING;
				//task che vanno schedulati normalmente
				pthread_setschedprio(tasks[SCHEDULE[frame_ind][i]].thread, (sched_get_priority_max(SCHED_FIFO) - 1 - i + rit));	//TODO modificare negli aperiodici
				TRACE_D("executive::scheduling normal periodic tasks", (sched_get_priority_max(SCHED_FIFO) - 1 - i + rit))
			} else {
				//task che risultano in ritardo
				pthread_setschedprio(tasks[SCHEDULE[frame_ind][i]].thread, (sched_get_priority_max(SCHED_FIFO) - dim + rit));
				TRACE_D("executive::scheduling late periodic tasks", (sched_get_priority_max(SCHED_FIFO) - dim + rit))
				++rit;
			}
			
			pthread_cond_signal(&tasks[SCHEDULE[frame_ind][i]].execute);
			///TEST: pthread_mutex_unlock(&tasks[SCHEDULE[frame_ind][i]].mutex);		//per proteggere lo stato e la variabile condizione del task
		}
			
			
			
		//non controllo se c'è una richiesta del task aperiodico perchè la mando al frame successivo
		
		++frame_count;
		frame_ind = frame_count % NUM_FRAMES;
		
		
		time.tv_sec = zero_time.tv_sec;
		time.tv_nsec = zero_time.tv_nsec;		//inizializzo il tempo a zero_time
		
		//sommo la quantità di nanosecondi che passa tra lo zero_time e l'inizio del prossimo frame (contando che frame_count è già stato incrementato)
		//TIME_UNIT_NS = 1e7 = 10^7 = dimensione del quanto temporale espressa in nanosecondi
		time.tv_nsec += (TIME_UNIT_NS * frame_dim) * frame_count;
		
		//normalizzo la struttura per riportarla in uno stato consistente
		time.tv_sec += ( time.tv_nsec ) / 1000000000;	
		time.tv_nsec = ( time.tv_nsec ) % 1000000000;
		
#ifndef	NDEBUG
		{
			struct timespec time_rel;
			time_rel.tv_sec = time.tv_sec;
			time_rel.tv_nsec = time.tv_nsec;
			TIME_DIFF(zero_time, time_rel)
			TRACE_L("executive_handler::waiting for next frame", time_rel.tv_sec)
			TRACE_L("executive_handler::waiting for next frame", time_rel.tv_nsec)
		}
#endif	//NDEBUG
		
		//mi metto in attesa che finisca il tempo del frame:
		pthread_mutex_lock(&executive.mutex);
		while(pthread_cond_timedwait(&executive.execute, &executive.mutex, &time) != ETIMEDOUT) ; 
		pthread_mutex_unlock(&executive.mutex);
	}
	return NULL;
}

int main(int argc, char** argv) {
	task_init();
	init();
	pthread_join(executive.thread, NULL);
	destroy();
	task_destroy();
}
