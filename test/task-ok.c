#include "task.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

/* Lunghezza dell'iperperiodo */
#define H_PERIOD_ 20

/* Numero di frame */
#define NUM_FRAMES_ 4

/* Numero di task */
#define NUM_P_TASKS_ 4

void task1_code();
void task2_code();
void task3_code();
void task4_code();


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
	
	/* Inizializzazione di P_TASKS[] */
	P_TASKS[1] = task1_code;
	P_TASKS[2] = task2_code;
	P_TASKS[3] = task3_code;
	P_TASKS[4] = task4_code;
	
	/* ... */

	/* Inizializzazione di AP_TASK */
	AP_TASK = ap_task_code;


	/* Inizializzazione di SCHEDULE e SLACK (se necessario) */

	/* frame 0 */
	SCHEDULE[0] = (int *) malloc( sizeof( int ) * 4 );
	SCHEDULE[0][0] = 1;
	SCHEDULE[0][1] = 2;
	SCHEDULE[0][2] = 4;
	SCHEDULE[0][3] = -1;

	SLACK[0] = 1; /* tutto il frame */


	/* frame 1 */
	SCHEDULE[1] = (int *) malloc( sizeof( int ) * 3 );
	SCHEDULE[1][0] = 3;
	SCHEDULE[1][1] = 2;
	SCHEDULE[1][2] = -1;

	SLACK[1] = 0; /* tutto il frame */


	/* frame 2 */
	SCHEDULE[2] = (int *) malloc( sizeof( int ) * 2 );
	SCHEDULE[2][0] = 1;
	SCHEDULE[2][1] = -1;

	SLACK[2] = 2; /* tutto il frame */


	/* frame 3 */
	SCHEDULE[3] = (int *) malloc( sizeof( int ) * 2 );
	SCHEDULE[3][0] = 2;
	SCHEDULE[3][1] = -1;

	SLACK[3] = 3; /* tutto il frame */
}

void task_destroy() {
	unsigned int i;

	/* Custom Code */
	for ( i = 0; i < NUM_FRAMES; ++i )
		free( SCHEDULE[i] );
}

/**********************************************************/

void busy_wait(unsigned int best, unsigned int worst) {
	struct timeval actual;
	struct timeval final;
	
	gettimeofday(&actual, NULL);
	
	unsigned int millisec = best + rand() % (worst - best + 1);
	
	final.tv_sec = actual.tv_sec + (actual.tv_usec + millisec * 1000) / 1000000;
	final.tv_usec = (actual.tv_usec + millisec * 1000) % 1000000;
	
	do {
		gettimeofday(&actual, NULL);
	} while (actual.tv_sec < final.tv_sec || (actual.tv_sec == final.tv_sec && actual.tv_usec < final.tv_usec));
}

/**********************************************************/

/* Nota: nel codice dei task e' lecito chiamare ap_task_request() */

void task1_code() {
	/* Custom Code */
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task1 started @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
	busy_wait(15, 20);
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "\t\ttask1 ended @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
}

void task2_code() {
	/* Custom Code */
	static unsigned int count = 0;
	struct timespec t;
	
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task2 started @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
	
	busy_wait(5, 10);
	
	if(count == 1) {
		ap_task_request();
		fprintf(stderr, "\t\ttask2 requested aperiodic task\n", t.tv_sec, t.tv_nsec);
	}
	
	count = (count + 1) % 3;
	
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "\t\ttask2 ended @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
}

void task3_code() {
	/* Custom Code */
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task3 started @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
	busy_wait(20, 40);
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "\t\ttask3 ended @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
}

void task4_code() {
	/* Custom Code */
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> task4 started @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
	busy_wait(5, 10);
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "\t\ttask4 ended @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
}

void ap_task_code() {
	/* Custom Code */
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "----> aperiodic task started @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
	busy_wait(40, 50);
	clock_gettime(CLOCK_REALTIME, &t);
	TIME_DIFF(zero_time, t)
	fprintf(stderr, "\t\taperiodic task ended @ (%ld)s (%ld)ns\n", t.tv_sec, t.tv_nsec);
}
