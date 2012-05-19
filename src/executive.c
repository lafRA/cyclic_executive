/* traccia dell'executive (pseudocodice) */
#include "executive-config.h"
#include "task.h"


//come faccio a specificare la funzione da eseguire??
void ap_task_request() {
// 	...
}

void p_task_handler(/*...*/) {
// 	...
}

void ap_task_handler(/*...*/) {
// 	...
}

void executive(/*...*/) {
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
