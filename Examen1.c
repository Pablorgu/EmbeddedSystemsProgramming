#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>


#define PERIODO_A_SEC 1
#define PERIODO_A_NSEC 0
#define PRIORIDAD_A 24
#define INC_A 1

#define PERIODO_B_SEC 2
#define PERIODO_B_NSEC 0
#define PRIORIDAD_B 26//POR QUE HA PUESTO PRIORIDAD MAYOR AL A
#define INC_B 2

#define PRIORIDAD_C 24

struct mutexAB{
    int contA;
    pthread_mutex_t mutexA;
    int contB;
    pthread_mutex_t mutexB;
};

void ini_mutexAB(struct mutexAB *data){
    pthread_mutex_init(&data->mutexA, NULL);
    pthread_mutex_init(&data->mutexB, NULL);
    data->contA = 0;
    data->contB = 0;
}

void *tarea_a(void *arg) {
    struct mutexAB* data = arg; //por que mutex ab es igual a arg

    const struct timespec periodo = {PERIODO_A_SEC, PERIODO_A_NSEC};

    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;

    int signum;

    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.sigev_signo = SIGRTMIN+1;
    sgev.sigev_value.sival_ptr = &timerid;

    timer_create(CLOCK_MONOTONIC, &sgev, &timerid);

    its.it_interval = periodo;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    timer_settime(timerid, 0, &its, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+1);

    while(1) {
        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexA);

        data->contA += INC_A;

        printf("Tarea A [%d]\n", data->contA);

        if(data->contA % 10 == 0){
            kill(getpid(), (SIGRTMIN+3)); //por que se mata al proceso
        }
        pthread_mutex_unlock(&data->mutexA);
    }
    timer_delete(timerid);
    return NULL;
}

void *tarea_b(void *arg) {
    struct mutexAB* data = arg;

    const struct timespec periodo = {PERIODO_B_SEC, PERIODO_B_NSEC};

    timer_t timerid;
    struct sigevent sgev;
    struct itimerspec its;
    sigset_t sigset;

    int signum;

    sgev.sigev_notify = SIGEV_SIGNAL;
    sgev.sigev_signo = SIGRTMIN+2;
    sgev.sigev_value.sival_ptr = &timerid;

    timer_create(CLOCK_MONOTONIC,&sgev, &timerid);

    its.it_interval = periodo;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    timer_settime(timerid, 0, &its, NULL);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+2);

    while (1) {
        sigwait(&sigset, &signum);
        pthread_mutex_lock(&data->mutexB);

        data->contB+= INC_B;

        printf("Tarea B: [%d]\n", data ->contB);

        if(data->contB % 5 == 0) {
            kill(getpid(), (SIGRTMIN+4));
        }
        pthread_mutex_unlock(&data->mutexB);
    }
    timer_delete(timerid);
    return NULL; 
}

void *tarea_c(void *arg){

    sigset_t sigset;
    int signum;

    struct mutexAB* data = arg;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);

    while(1) {
        sigwait(&sigset, &signum);

        pthread_mutex_lock(&data->mutexA);
        pthread_mutex_lock(&data->mutexB);

        if(signum == SIGRTMIN+3){
            printf("----Contador A multiplo de diez:[%d]\n", data->contA);
        }
        if(signum == SIGRTMIN+4){
            printf("----Contador B multiplo de cinco: [%d]\n", data->contB);
        }

        pthread_mutex_unlock(&data->mutexB);
        pthread_mutex_unlock(&data->mutexA);
    }
    return NULL;
}

int main(int argc, const char *argv[]) {
    sigset_t sigset;

    struct mutexAB data;
    struct sched_param param;
    pthread_attr_t attr;


    int prio0 = 30, prio1 = PRIORIDAD_A, prio2 = PRIORIDAD_B, prio3 = PRIORIDAD_C;
    pthread_t t1, t2, t3;


    mlockall(MCL_CURRENT | MCL_FUTURE);
    param.sched_priority = prio0;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGRTMIN+1);
    sigaddset(&sigset, SIGRTMIN+2);
    sigaddset(&sigset, SIGRTMIN+3);
    sigaddset(&sigset, SIGRTMIN+4);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    ini_mutexAB(&data);

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    param.sched_priority = prio1;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t1, &attr, tarea_a, &data);

    param.sched_priority = prio2;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t2, &attr, tarea_b, &data);

    param.sched_priority = prio3;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&t3, &attr, tarea_c, &data);

    pthread_attr_destroy(&attr);

    printf("Tarea principal con politica fifo");
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_mutex_destroy(&data.mutexA);
    pthread_mutex_destroy(&data.mutexB);
    return 0;
}