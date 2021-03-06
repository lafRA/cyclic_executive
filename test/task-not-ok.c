#include "task.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* Lunghezza dell'iperperiodo */
#define H_PERIOD_ 25

/* Numero di frame */
#define NUM_FRAMES_ 5

/* Numero di task */
#define NUM_P_TASKS_ 6

void task1_code();
void task2_code();
void task3_code();
void task4_code();
void task5_code();
void task6_code();


void ap_task_code();

/**********************/

/* Questo inizializza i dati globali */
const unsigned int H_PERIOD = H_PERIOD_;
const unsigned int NUM_FRAMES = NUM_FRAMES_;
const unsigned int NUM_P_TASKS = NUM_P_TASKS_;

task_routine P_TASKS[NUM_P_TASKS_];
task_routine AP_TASK;
int * SCHEDULE[NUM_FRAMES_];
int SLACK[NUM_FRAMES_];

void task_init() {
	fprintf(stderr, "task-ok.c: initializing the task set\n");
	
	srand(getpid());
	
	/* Inizializzazione di P_TASKS[] */
	P_TASKS[0] = task1_code;
	P_TASKS[1] = task2_code;
	P_TASKS[2] = task3_code;
	P_TASKS[3] = task4_code;
	P_TASKS[4] = task5_code;
	P_TASKS[5] = task6_code;
	
	/* ... */

	/* Inizializzazione di AP_TASK */
	AP_TASK = ap_task_code;


	/* Inizializzazione di SCHEDULE e SLACK (se necessario) */

	/* frame 0 */
	SCHEDULE[0] = (int *) malloc( sizeof( int ) * 4 );
	SCHEDULE[0][0] = 0;
	SCHEDULE[0][1] = 1;				//richiesta per task aperiodico
 	SCHEDULE[0][2] = 4;
	SCHEDULE[0][3] = -1;

	SLACK[0] = 0;


	/* frame 1 */
	
	//ci aspettiamo una deadline miss del task5
	
	SCHEDULE[1] = (int *) malloc( sizeof( int ) * 3 );
	SCHEDULE[1][0] = 4;
	SCHEDULE[1][1] = 0;
	SCHEDULE[1][2] = -1;

	SLACK[1] = 0;


	/* frame 2 */
	SCHEDULE[2] = (int *) malloc( sizeof( int ) * 3 );
	SCHEDULE[2][0] = 1;				//richiesta per task aperiodico
	SCHEDULE[2][1] = 2;
	SCHEDULE[2][2] = -1;

	SLACK[2] = 0;


	/* frame 3 */
	SCHEDULE[3] = (int *) malloc( sizeof( int ) * 2 );
	SCHEDULE[3][0] = 0;		
	SCHEDULE[3][1] = -1;
	
	SLACK[3] = 3;
	
	/* frame 4 */
	SCHEDULE[4] = (int *) malloc( sizeof( int ) * 2 );
	SCHEDULE[4][0] = 0;
	SCHEDULE[4][1] = -1;
	SLACK[4] = 3;

}

void task_destroy() {
	unsigned int i;

	/* Custom Code */
	for ( i = 0; i < NUM_FRAMES; ++i )
		free( SCHEDULE[i] );
}

/**********************************************************/

// void busy_wait(unsigned int best, unsigned int worst) {
// 	struct timeval actual;
// 	struct timeval final;
// 	
// 	gettimeofday(&actual, NULL);
// 	
// 	unsigned int millisec = best + rand() % (worst - best + 1);
// 	
// 	final.tv_sec = actual.tv_sec + (actual.tv_usec + millisec * 1000) / 1000000;
// 	final.tv_usec = (actual.tv_usec + millisec * 1000) % 1000000;
// 	
// 	do {
// 		gettimeofday(&actual, NULL);
// 	} while (actual.tv_sec < final.tv_sec || (actual.tv_sec == final.tv_sec && actual.tv_usec < final.tv_usec));
// }

/**
 * La vecchia versione del busy-wait non funziona nel caso in cui il task subisca preemption (perchè il tempo continua a scorrere).
 * Per modellare l'effettiva esecuzione di un dato intervallo temporale è necessario usare il clock CLOCK_THREAD_CPUTIME_ID.
 * Inoltre per rendere la simulazione più realistica il tempo d'attesa è specificato in termini di tempo minimo e massimo (e viene distribuito uniformemente tra i due per semplicità)
 */

void busy_wait(unsigned int best, unsigned int worst) {
	struct timespec actual;
	struct timespec final;
	
// 	gettimeofday(&actual, NULL);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &actual);
	
	unsigned int millisec = best + rand() % (worst - best + 1);
	
	final.tv_sec = actual.tv_sec + (actual.tv_nsec + millisec * 1000000) / 1000000000;
	final.tv_nsec = (actual.tv_nsec + millisec * 1000000) % 1000000000;
	
	do {
// 		gettimeofday(&actual, NULL);
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &actual);
	} while (actual.tv_sec < final.tv_sec || (actual.tv_sec == final.tv_sec && actual.tv_nsec < final.tv_nsec));
}

/**********************************************************/

/* Nota: nel codice dei task e' lecito chiamare ap_task_request() */

void task1_code() {
	/* Custom Code */
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task1 started @ (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6);
	busy_wait(10, 15);
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\ttask1 ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}

void task2_code() {
	/* Custom Code */
	
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task2 started @ (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6);
	busy_wait(15, 20);
// 	if(count == 1) {
		ap_task_request();
		fprintf(stderr, "\t\ttask2 requested aperiodic task\n");
// 	}
// 	count = (count + 1) % 3;
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\ttask2 ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}

void task3_code() {
	/* Custom Code */
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task3 started @ (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6);
	busy_wait(20, 25);
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\ttask3 ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}

void task4_code() {
	/* Custom Code */
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task4 started @ (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6);
	busy_wait(25, 30);
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\ttask4 ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}

void task5_code() {
	/* Custom Code */
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t);
	busy_wait(30, 35);
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\ttask5 ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}

void task6_code() {
	/* Custom Code */
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t);
	busy_wait(40, 45);
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\ttask6 ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}

void ap_task_code() {
	/* Custom Code */
	struct timespec t, start, end;		//da notare che t viene usato per indicare gli istati di partenza e fine ASSOLUTI, mentre start e end si riferiscono all'effettivo tempo di esecuzione del thread in questione
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> aperiodic task started @ (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6);
	busy_wait(40, 50);
	clock_gettime(CLOCK_REALTIME, &t);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	TIME_DIFF(zero_time, t)
	TIME_DIFF(start, end)
	fprintf(stderr, "\t\taperiodic task ended @ (%ld)s (%.3f)ms\t||\t actual execution time: (%ld)s (%.3f)ms\n", t.tv_sec, t.tv_nsec/1e6, end.tv_sec, end.tv_nsec/1e6);
}
