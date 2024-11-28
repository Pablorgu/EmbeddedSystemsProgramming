#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include<time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>

#define PRIORIDAD_A 24
#define ITER_A 40
#define INC_A 100

#define PRIORIDAD_B 26
#define ITER_B 40
#define INC_B 1

struct Data{
    pthread_mutex_t mutex;
    int cnt;
}

void espera_activa(time_t seg){
    volatile time_t t = time(0) + seg;
    while (time(0) < t){ /*Esperar hasta que el tiempo actual sea mayor que t*/}
}

void *tarea_a(void *arg){
    struct Data *data = arg;
    struct sched_param param;
    const char *pol;
    int i;
    int policy;
    int prio0 = 1, prio1=PRIORIDAD_A, 
    pthread_getschedparam(pthread_self(), &policy, $param);
    pol= SCHED_FIFO;

    
}

int main(int argc) {
    sigset_t sigset;
    struct Data shared_data;
    pthread_attr_t attr;
    const char *pol;
    int pcy, policy = SCHED_FIFO;
    int prio0=1, prio1=1, prio2=1;
    pthread_t t1, t2;

    
}