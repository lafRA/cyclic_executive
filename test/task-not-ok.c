#include "task.h"

#include <stdlib.h>
#include <sys/time.h>

/* Lunghezza dell'iperperiodo */
#define H_PERIOD_ 20

/* Numero di frame */
#define NUM_FRAMES_ 5

/* Numero di task */
#define NUM_P_TASKS_ 2

void task0_code();
void task1_code();

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

void task_init()
  {
  /* Inizializzazione di P_TASKS[] */
  P_TASKS[0] = task0_code;
  P_TASKS[1] = task1_code;
  /* ... */
  
  /* Inizializzazione di AP_TASK */
  AP_TASK = ap_task_code;


  /* Inizializzazione di SCHEDULE e SLACK (se necessario) */

  /* frame 0 */
  SCHEDULE[0] = (int *) malloc( sizeof( int ) * 1 );
  SCHEDULE[0][0] = -1;

  SLACK[0] = 4; /* tutto il frame */


  /* frame 1 */
  SCHEDULE[1] = (int *) malloc( sizeof( int ) * 1 );
  SCHEDULE[1][0] = -1;

  SLACK[1] = 4; /* tutto il frame */


  /* frame 2 */
  SCHEDULE[2] = (int *) malloc( sizeof( int ) * 1 );
  SCHEDULE[2][0] = -1;

  SLACK[2] = 4; /* tutto il frame */


  /* frame 3 */
  SCHEDULE[3] = (int *) malloc( sizeof( int ) * 1 );
  SCHEDULE[3][0] = -1;

  SLACK[3] = 4; /* tutto il frame */


  /* frame 4 */
  SCHEDULE[4] = (int *) malloc( sizeof( int ) * 1 );
  SCHEDULE[4][0] = -1;
  
  SLACK[4] = 4; /* tutto il frame */

  /* Custom Code */
  }

void task_destroy()
  {
  unsigned int i;

  /* Custom Code */

  for ( i = 0; i < NUM_FRAMES; ++i )
    free( SCHEDULE[i] );
  }

/**********************************************************/

void busy_wait(unsigned int millisec)
  {
  struct timeval actual;
  struct timeval final;

  gettimeofday(&actual, NULL);

  final.tv_sec = actual.tv_sec + (actual.tv_usec + millisec * 1000) / 1000000;
  final.tv_usec = (actual.tv_usec + millisec * 1000) % 1000000;

  do
    {
    gettimeofday(&actual, NULL);
    }
  while (actual.tv_sec < final.tv_sec || (actual.tv_sec == final.tv_sec && actual.tv_usec < final.tv_usec));
  }
  
/**********************************************************/

/* Nota: nel codice dei task e' lecito chiamare ap_task_request() */

 void task0_code()
   {
   /* Custom Code */
   }

  void task1_code()
   {
   /* Custom Code */
   }


  void ap_task_code()
   {
   /* Custom Code */
   }
